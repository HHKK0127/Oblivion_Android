package com.example.oblivion;

import android.media.MediaCodec;
import android.media.MediaExtractor;
import android.media.MediaFormat;
import android.media.PlaybackParams;
import android.os.Build;
import android.util.Log;
import android.view.Surface;

import java.io.IOException;
import java.nio.ByteBuffer;

/**
 * VideoBridge - Java-side MediaCodec wrapper for Bink Video replacement.
 * Phase 53: Video playback system for Oblivion Android.
 *
 * This class wraps Android MediaCodec to provide hardware-accelerated
 * video decoding, bridged to native C++ via JNI callbacks.
 */
public class VideoBridge {

    private static final String TAG = "VideoBridge";

    // MediaCodec components
    private MediaExtractor extractor;
    private MediaCodec decoder;
    private Surface outputSurface;

    // State
    private volatile boolean decoding = false;
    private volatile boolean paused = false;
    private volatile boolean released = false;

    // Video info
    private int videoWidth = 0;
    private int videoHeight = 0;
    private float frameRate = 30.0f;
    private long durationUs = 0;

    // Audio
    private float volume = 1.0f;

    // Native handle for callbacks
    private long nativeHandle = 0;

    // Decode thread
    private Thread decodeThread;

    // Timing
    private long startPresentationTimeUs = 0;
    private long lastFrameTimeUs = 0;

    /**
     * Create a MediaCodec decoder for the given video file.
     *
     * @param filePath Path to the video file on device storage
     * @param surface  Output Surface for decoded frames (can be null for SurfaceTexture mode)
     * @return true if decoder was created successfully
     */
    public boolean createDecoder(String filePath, Surface surface) {
        if (released) {
            Log.e(TAG, "createDecoder: already released");
            return false;
        }

        Log.i(TAG, "Creating decoder for: " + filePath);

        try {
            // Create MediaExtractor to read the container
            extractor = new MediaExtractor();
            extractor.setDataSource(filePath);

            // Find the video track
            int videoTrackIndex = findVideoTrack(extractor);
            if (videoTrackIndex < 0) {
                Log.e(TAG, "No video track found in: " + filePath);
                release();
                return false;
            }

            // Select the video track
            extractor.selectTrack(videoTrackIndex);

            // Get video format info
            MediaFormat format = extractor.getTrackFormat(videoTrackIndex);
            String mimeType = format.getString(MediaFormat.KEY_MIME);

            videoWidth = format.getInteger(MediaFormat.KEY_WIDTH);
            videoHeight = format.getInteger(MediaFormat.KEY_HEIGHT);
            durationUs = format.containsKey(MediaFormat.KEY_DURATION)
                    ? format.getLong(MediaFormat.KEY_DURATION) : 0;

            if (format.containsKey(MediaFormat.KEY_FRAME_RATE)) {
                frameRate = format.getInteger(MediaFormat.KEY_FRAME_RATE);
            }

            Log.i(TAG, String.format("Video: %dx%d, %.1f fps, mime=%s, duration=%d us",
                    videoWidth, videoHeight, frameRate, mimeType, durationUs));

            // Create MediaCodec decoder
            decoder = MediaCodec.createDecoderByType(mimeType);

            if (surface != null) {
                outputSurface = surface;
            } else {
                // In SurfaceTexture mode, the surface will be provided later
                Log.w(TAG, "No output surface provided, decoder created without surface");
                // For now, we need a surface to configure the decoder
                // This will be set up when the native side provides the ANativeWindow
                Log.e(TAG, "Surface is required for MediaCodec configuration");
                release();
                return false;
            }

            // Configure decoder with output surface
            decoder.configure(format, outputSurface, null, 0);

            Log.i(TAG, "Decoder created successfully");
            return true;

        } catch (IOException e) {
            Log.e(TAG, "IOException creating decoder: " + e.getMessage(), e);
            release();
            return false;
        } catch (MediaCodec.CodecException e) {
            Log.e(TAG, "CodecException creating decoder: " + e.getMessage(), e);
            release();
            return false;
        } catch (IllegalArgumentException e) {
            Log.e(TAG, "IllegalArgumentException creating decoder: " + e.getMessage(), e);
            release();
            return false;
        }
    }

    /**
     * Start the asynchronous decode loop.
     *
     * @return true if decoding started successfully
     */
    public boolean startDecoding() {
        if (decoder == null || released) {
            Log.e(TAG, "startDecoding: decoder not created or released");
            return false;
        }

        if (decoding) {
            Log.w(TAG, "startDecoding: already decoding");
            return true;
        }

        try {
            decoder.start();
            decoding = true;
            paused = false;

            // Notify native side that decoder is ready
            if (nativeHandle != 0) {
                nativeOnDecoderReady(nativeHandle, videoWidth, videoHeight, frameRate);
            }

            // Start decode thread
            decodeThread = new Thread(this::decodeLoop, "VideoDecodeThread");
            decodeThread.setPriority(Thread.MAX_PRIORITY);
            decodeThread.start();

            Log.i(TAG, "Decoding started");
            return true;

        } catch (MediaCodec.CodecException e) {
            Log.e(TAG, "CodecException starting decoder: " + e.getMessage(), e);
            notifyError(-3, "Failed to start decoder: " + e.getMessage());
            return false;
        }
    }

    /**
     * Pause decoding (keeps decoder alive).
     */
    public void pauseDecoding() {
        if (!decoding || paused) {
            return;
        }
        paused = true;
        Log.d(TAG, "Decoding paused");
    }

    /**
     * Resume decoding after pause.
     */
    public void resumeDecoding() {
        if (!decoding || !paused) {
            return;
        }
        paused = false;
        synchronized (this) {
            notifyAll();  // Wake up decode thread
        }
        Log.d(TAG, "Decoding resumed");
    }

    /**
     * Stop decoding and release MediaCodec resources.
     */
    public void stopDecoding() {
        decoding = false;
        paused = false;

        // Wake up decode thread if waiting
        synchronized (this) {
            notifyAll();
        }

        // Wait for decode thread to finish
        if (decodeThread != null) {
            try {
                decodeThread.join(2000);  // Wait up to 2 seconds
            } catch (InterruptedException e) {
                Log.w(TAG, "Interrupted waiting for decode thread");
                Thread.currentThread().interrupt();
            }
            decodeThread = null;
        }

        // Stop and release decoder
        if (decoder != null) {
            try {
                decoder.stop();
            } catch (Exception e) {
                Log.w(TAG, "Exception stopping decoder: " + e.getMessage());
            }
        }

        Log.d(TAG, "Decoding stopped");
    }

    /**
     * Set audio volume (0.0 to 1.0).
     */
    public void setVolume(float vol) {
        volume = Math.max(0.0f, Math.min(1.0f, vol));
        // Volume control for audio track would be handled here
        // if audio decoding is implemented
    }

    /**
     * Set playback rate.
     */
    public void setPlaybackRate(float rate) {
        if (decoder != null && Build.VERSION.SDK_INT >= Build.VERSION_CODES.M) {
            try {
                PlaybackParams params = new PlaybackParams();
                params.setSpeed(Math.max(0.25f, Math.min(4.0f, rate)));
                // Use reflection for API compatibility
                java.lang.reflect.Method method = decoder.getClass().getMethod("setPlaybackParams", PlaybackParams.class);
                method.invoke(decoder, params);
            } catch (Exception e) {
                Log.w(TAG, "Failed to set playback rate: " + e.getMessage());
            }
        }
    }

    /**
     * Release all resources.
     */
    public void release() {
        if (released) {
            return;
        }
        released = true;
        decoding = false;

        if (decoder != null) {
            try {
                decoder.stop();
                decoder.release();
            } catch (Exception e) {
                Log.w(TAG, "Exception releasing decoder: " + e.getMessage());
            }
            decoder = null;
        }

        if (extractor != null) {
            try {
                extractor.release();
            } catch (Exception e) {
                Log.w(TAG, "Exception releasing extractor: " + e.getMessage());
            }
            extractor = null;
        }

        if (outputSurface != null) {
            try {
                outputSurface.release();
            } catch (Exception e) {
                Log.w(TAG, "Exception releasing surface: " + e.getMessage());
            }
            outputSurface = null;
        }

        Log.i(TAG, "VideoBridge released");
    }

    /**
     * Set the native handle for JNI callbacks.
     */
    public void setNativeHandle(long handle) {
        this.nativeHandle = handle;
    }

    // =========================================================================
    // Decode loop (runs on dedicated thread)
    // =========================================================================

    private void decodeLoop() {
        Log.d(TAG, "Decode loop started");

        MediaCodec.BufferInfo bufferInfo = new MediaCodec.BufferInfo();
        boolean inputDone = false;
        boolean outputDone = false;
        long timeoutUs = 10000;  // 10ms timeout

        while (decoding && !outputDone) {
            // Handle pause
            if (paused) {
                synchronized (this) {
                    try {
                        wait();  // Block until resumed
                    } catch (InterruptedException e) {
                        Thread.currentThread().interrupt();
                        break;
                    }
                }
                continue;
            }

            // Feed input data
            if (!inputDone) {
                int inputIndex = decoder.dequeueInputBuffer(timeoutUs);
                if (inputIndex >= 0) {
                    ByteBuffer inputBuffer = decoder.getInputBuffer(inputIndex);
                    if (inputBuffer != null) {
                        int sampleSize = extractor.readSampleData(inputBuffer, 0);
                        if (sampleSize < 0) {
                            // End of input stream
                            decoder.queueInputBuffer(inputIndex, 0, 0, 0,
                                    MediaCodec.BUFFER_FLAG_END_OF_STREAM);
                            inputDone = true;
                            Log.d(TAG, "End of input stream");
                        } else {
                            long presentationTimeUs = extractor.getSampleTime();
                            decoder.queueInputBuffer(inputIndex, 0, sampleSize,
                                    presentationTimeUs, 0);
                            extractor.advance();
                        }
                    }
                }
            }

            // Drain output
            int outputIndex = decoder.dequeueOutputBuffer(bufferInfo, timeoutUs);
            if (outputIndex >= 0) {
                // Check for end of stream
                if ((bufferInfo.flags & MediaCodec.BUFFER_FLAG_END_OF_STREAM) != 0) {
                    outputDone = true;
                    Log.d(TAG, "End of output stream");
                }

                // Release buffer to render to surface
                decoder.releaseOutputBuffer(outputIndex, true);

                // Notify native side of decoded frame
                if (nativeHandle != 0 && !outputDone) {
                    lastFrameTimeUs = bufferInfo.presentationTimeUs;
                    nativeOnFrameDecoded(nativeHandle, bufferInfo.presentationTimeUs);
                }

            } else if (outputIndex == MediaCodec.INFO_OUTPUT_FORMAT_CHANGED) {
                MediaFormat newFormat = decoder.getOutputFormat();
                Log.d(TAG, "Output format changed: " + newFormat);
            }

            // Small sleep to prevent busy-waiting
            if (!inputDone && outputIndex < 0) {
                try {
                    Thread.sleep(1);
                } catch (InterruptedException e) {
                    Thread.currentThread().interrupt();
                    break;
                }
            }
        }

        // Notify end of stream
        if (nativeHandle != 0) {
            nativeOnEndOfStream(nativeHandle);
        }

        Log.d(TAG, "Decode loop ended");
    }

    // =========================================================================
    // Helper methods
    // =========================================================================

    /**
     * Find the first video track in the media file.
     */
    private int findVideoTrack(MediaExtractor extractor) {
        for (int i = 0; i < extractor.getTrackCount(); i++) {
            MediaFormat format = extractor.getTrackFormat(i);
            String mime = format.getString(MediaFormat.KEY_MIME);
            if (mime != null && mime.startsWith("video/")) {
                return i;
            }
        }
        return -1;
    }

    /**
     * Notify native side of an error.
     */
    private void notifyError(int errorCode, String message) {
        if (nativeHandle != 0) {
            nativeOnDecodeError(nativeHandle, errorCode, message);
        }
    }

    // =========================================================================
    // Native callbacks (C++ -> Java -> C++)
    // =========================================================================

    /**
     * Called from C++ to set the native decoder handle.
     * This is used for JNI callbacks from Java to C++.
     */
    public static native void nativeOnFrameDecoded(long decoderHandle, long presentationTimeUs);

    public static native void nativeOnEndOfStream(long decoderHandle);

    public static native void nativeOnDecodeError(long decoderHandle, int errorCode, String message);

    public static native void nativeOnDecoderReady(long decoderHandle, int width, int height, float frameRate);
}
