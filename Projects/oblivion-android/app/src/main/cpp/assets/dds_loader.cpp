#include <string>
#include <android/log.h>

#define LOG_TAG_DDS "DdsLoader"
#define LOGD(...) __android_log_print(ANDROID_LOG_DEBUG, LOG_TAG_DDS, __VA_ARGS__)

// ============================================================================
// DDS Loader Stub - DirectDraw Surface texture format
// ============================================================================
// DDS = DirectDraw Surface format used by Oblivion textures
// Full implementation will parse .dds files and upload to OpenGL

bool loadDDSFile(const std::string& path, unsigned int& outTextureId) {
    LOGD("DDS load stub: %s", path.c_str());
    outTextureId = 0;
    return false;
}