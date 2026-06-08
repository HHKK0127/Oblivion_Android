#ifndef AL_AL_H
#define AL_AL_H

#include <cstdint>

typedef uint32_t ALuint;
typedef int32_t  ALint;
typedef int32_t  ALsizei;
typedef int32_t  ALenum;

#define AL_FORMAT_MONO8     0x1100
#define AL_FORMAT_MONO16    0x1101
#define AL_FORMAT_STEREO8   0x1102
#define AL_FORMAT_STEREO16  0x1103
#define AL_NONE             0
#define AL_FALSE            0
#define AL_TRUE             1
#define AL_NO_ERROR         0
#define AL_PLAYING          0x1012
#define AL_STOPPED          0x1014
#define AL_LOOPING          0x1007
#define AL_GAIN             0x100A
#define AL_POSITION         0x1004
#define AL_VELOCITY         0x1006
#define AL_ORIENTATION      0x100F
#define AL_SOURCE_STATE     0x1010
#define AL_BUFFER           0x1009
#define AL_PITCH            0x1003
#define AL_SOURCE_RELATIVE  0x0202
#define AL_VENDOR           0xB001
#define AL_VERSION          0xB002
#define AL_INVERSE_DISTANCE 0xD001
#define AL_INVERSE_DISTANCE_CLAMPED 0xD002
#define AL_LINEAR_DISTANCE  0xD003
#define AL_EXPONENT_DISTANCE 0xD004

inline void alDopplerFactor(float) {}
inline void alSpeedOfSound(float) {}
inline void alDistanceModel(ALenum) {}
inline const char* alGetString(ALenum) { return ""; }
inline void alListenerfv(ALint, const float*) {}

inline void alGenBuffers(ALsizei, ALuint*) {}
inline void alDeleteBuffers(ALsizei, const ALuint*) {}
inline void alBufferData(ALuint, ALint, const void*, ALsizei, ALsizei) {}
inline void alGenSources(ALsizei, ALuint*) {}
inline void alDeleteSources(ALsizei, const ALuint*) {}
inline void alSourcePlay(ALuint) {}
inline void alSourceStop(ALuint) {}
inline void alSourcePause(ALuint) {}
inline void alSourcei(ALuint, ALint, ALint) {}
inline void alSourcef(ALuint, ALint, float) {}
inline void alSource3f(ALuint, ALint, float, float, float) {}
inline void alGetSourcei(ALuint, ALint, ALint*) {}
inline void alGetSourcef(ALuint, ALint, float*) {}
inline void alListener3f(ALint, float, float, float) {}
inline void alListenerf(ALint, float) {}
inline ALenum alGetError() { return AL_FALSE; }

#endif
