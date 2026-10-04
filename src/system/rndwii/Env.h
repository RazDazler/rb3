#pragma once

#include "bandobj/BandCharacter.h"
#include "obj/ObjMacros.h"
#include "obj/ObjPtr_p.h"
#include "rndobj/Env.h"
#include "rndobj/Mat.h"
#include "rndobj/Cam.h"
#include "rndwii/Lit.h"

class WiiEnviron : public RndEnviron {
public:
    WiiEnviron();
    virtual ~WiiEnviron();
#ifdef VERSION_SZBE69_B8
    OBJ_CLASSNAME(Environ)
#else
    OBJ_CLASSNAME(WiiEnviron)
#endif
    OBJ_SET_TYPE(WiiEnviron)

    bool SetLight(int, WiiLight *);
#ifdef VERSION_SZBE69_B8
    void ClearLights();
    virtual void Select(const Vector3 *);
    virtual void ApplyApproxLighting(const GXColor *);
    void SetDirLight(int, GXColor, const Vector3 &);
#endif
#ifdef VERSION_SZBE69_B8
    static void RenderCharactersToShadowBuffers();
#else
    void RenderCharactersToShadowBuffers();
#endif

    bool unk_0x19B, unk_0x19C;
    u16 unk_0x19E;

    static RndMat *mShadowMat;
    static RndCam *mShadowCam;
    static ObjPtrList<BandCharacter> mShadowedCharacters;
    static bool mbEnableShadows;
    static bool mbRenderingShadows;
    static bool mbShowShadowTextureOnScreen;
    static bool mShadowLightSet;
};
