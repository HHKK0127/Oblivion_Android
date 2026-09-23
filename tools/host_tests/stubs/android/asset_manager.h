#ifndef ANDROID_ASSET_MANAGER_H
#define ANDROID_ASSET_MANAGER_H

#include <cstddef>
#include <sys/types.h>

#ifdef __cplusplus
extern "C" {
#endif

typedef struct AAssetManager AAssetManager;
typedef struct AAsset AAsset;

enum {
    AASSET_MODE_UNKNOWN = 0,
    AASSET_MODE_RANDOM = 1,
    AASSET_MODE_STREAMING = 2,
    AASSET_MODE_BUFFER = 3
};

AAsset* AAssetManager_open(AAssetManager* mgr, const char* filename, int mode);
void AAsset_close(AAsset* asset);
off_t AAsset_getLength(AAsset* asset);
int AAsset_read(AAsset* asset, void* buf, size_t count);

#ifdef __cplusplus
}
#endif

#endif
