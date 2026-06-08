#ifndef AL_ALC_H
#define AL_ALC_H

#include "al.h"

typedef void ALCdevice;
typedef void ALCcontext;
typedef char ALCboolean;
typedef int32_t ALCenum;

#define ALC_DEVICE_SPECIFIER 0x1005

inline ALCdevice* alcOpenDevice(const char*) { return nullptr; }
inline void alcCloseDevice(ALCdevice*) {}
inline ALCcontext* alcCreateContext(ALCdevice*, const int*) { return nullptr; }
inline ALCboolean alcMakeContextCurrent(ALCcontext*) { return 1; }
inline void alcDestroyContext(ALCcontext*) {}
inline ALCcontext* alcGetCurrentContext() { return nullptr; }
inline const char* alcGetString(ALCdevice*, ALCenum) { return ""; }

#endif
