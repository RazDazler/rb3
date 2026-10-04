#include "synth/Utl.h"

float CalcSpeedFromTranspose(float f) { return std::pow(2.0f, f / 12.0f); }
#ifdef VERSION_SZBE69
#include "os/File.h"
#include "os/System.h"
#include "utl/MakeString.h"
#include <string.h>

// This platform helper forwards the incoming path to the disc lookup routine.
extern bool fn_802E8F88(const char *);
static char gCacheWavName[256];

DECOMP_FORCEACTIVE(
    Utl, "sixteenth", "eighth", "dotted_eighth", "quarter", "dotted_quarter", "half", "whole"
)

const char *CacheWav(const char *path, CacheResourceResult &result) {
    result = kCacheUnnecessary;
    Platform platform = kPlatformWii;
    if (!path || !*path || platform == kPlatformNone)
        return 0;
    if (platform == kPlatformPC)
        return path;
    path = FileLocalize(path, 0);
    strcpy(
        gCacheWavName,
        MakeString(
            "%s/gen/%s.%s_%s",
            FileGetPath(path, 0),
            FileGetBase(path, 0),
            FileGetExt(path),
            PlatformSymbol(platform)
        )
    );
    FileIsLocal(path);
    fn_802E8F88(gCacheWavName);
    return gCacheWavName;
}
#endif
