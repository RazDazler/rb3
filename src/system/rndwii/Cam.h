#pragma once

#include "obj/ObjMacros.h"
#include "rndobj/Cam.h"

class WiiCam : public RndCam {
#ifdef VERSION_SZBE69_B8
public:
#endif
    WiiCam();
    OBJ_CLASSNAME(WiiCam)
    OBJ_SET_TYPE(WiiCam)
    virtual void Select();
    virtual u32 ProjectZ(float);

#ifdef VERSION_SZBE69_B8
    // Selected Wii view transform; B8 constructor places the virtual base at 0x2a8.
    Transform mWiiViewXfm; // 0x278..0x2a7
#endif
    static Transform sViewToWiiViewXfm;
    static Transform sWiiViewToViewXfm;
};
