#pragma once
#include <vector>
class DeJitter {
public:
    DeJitter();
    float Apply(float, float &);
    void Reset();

    std::vector<float> unk_0x0;
    int unk_0x8, unk_0xC;
    float unk_0x10, unk_0x14;
#ifdef MILO_DEBUG
    float unk_0x18;
#endif

    // bank 5 says:
    // mJitter, mJitterIndex, mJitterWindow, mLastAverage, mLastMs
    // these were found in GamePanel's members

    static float sTimeScale;
};
