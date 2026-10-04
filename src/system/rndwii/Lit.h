#pragma once

#include <revolution/GX.h>
#include "decomp.h"
#include "rndobj/Lit.h"

class WiiLight : public RndLight {
public:
    WiiLight() {}
    virtual ~WiiLight() {}
#ifdef VERSION_SZBE69_B8
    virtual Symbol ClassName() const { return StaticClassName(); }
#ifdef RB3_B8_FORCE_WIILIGHT_CLASSNAME
    FORCE_LOCAL_INLINE
#endif
    static Symbol StaticClassName() {
        static Symbol name("Light");
        return name;
    }
#ifdef RB3_B8_FORCE_WIILIGHT_CLASSNAME
END_FORCE_LOCAL_INLINE
#endif
#endif

void Update(GXLightID);
void UpdatePosition();
float GetLightFieldOfView();
Vector3 CalcAdjustedPos();

u16 _pad;
GXLightObj mLight; // 0x11C
bool unk_0x15C;
}
;
