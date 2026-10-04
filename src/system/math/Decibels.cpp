#include "math/Decibels.h"
#include "math/Utl.h"
#include "os/Debug.h"

float DbToRatio(float db) {
    float ratio;
    if (db <= -96.0f)
        ratio = 0.0f;
    else
#ifdef VERSION_SZBE69
        ratio = powf(10.0f, db / 20.0f);
#else
        ratio = std::pow(10.0f, db / 20.0f);
#endif
    return ratio;
}

float RatioToDb(float ratio) {
    if (ratio < 0.0f) {
        ratio = 0.0f;
        MILO_LOG("Got a BAD Decibel ratio\n");
    }
    return (ratio <= 0.0f) ? -96.0f : std::log10(ratio) * 20.0f;
}
