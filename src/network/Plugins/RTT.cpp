#include "RTT.h"

namespace Quazal {
    RTT::RTT(uint i) : unk_0x0(i * 8), unk_0x4(0), unk_0x8(i) {}

    RTT::~RTT() {}

    void RTT::Adjust(uint i) {
        uint smoothed;
        int sign;
        uint variance;
        int delta;
        smoothed = unk_0x0;
        variance = unk_0x4;
        unk_0x8 = i;
        delta = int(i - (smoothed >> 3));
        sign = delta >> 31;
        unk_0x0 = smoothed + delta;
        unk_0x4 = variance + ((uint(delta) ^ uint(sign)) - uint(sign)) - (variance >> 2);
    }
}