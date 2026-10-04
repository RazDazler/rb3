#include "Env.h"
#include "math/Color.h"
#include "obj/DataFunc.h"
#include "obj/ObjPtr_p.h"
#include "os/Debug.h"
#include "revolution/gx/GXTypes.h"
#ifdef VERSION_SZBE69_B8
#include "revolution/gx/GXLight.h"
#include "rndwii/Rnd.h"
#include "rndwii/Cam.h"
#include "rndobj/BoxMap.h"
#endif
#include "rndobj/Lit.h"
#include "rndobj/Mat.h"

ObjPtrList<BandCharacter> WiiEnviron::mShadowedCharacters(NULL, kObjListNoNull);
RndMat *WiiEnviron::mShadowMat;
RndCam *WiiEnviron::mShadowCam;
#ifndef MILO_DEBUG
Transform some_xform;
#endif
bool WiiEnviron::mShadowLightSet;
bool WiiEnviron::mbRenderingShadows;
bool WiiEnviron::mbEnableShadows;
bool WiiEnviron::mbShowShadowTextureOnScreen;

static GXLightID LightId(int n) {
#ifdef VERSION_SZBE69_B8
    MILO_ASSERT(0 <= n && n < 8, 34);
#else
    MILO_ASSERT_RANGE(n, 0, 8, 34);
#endif
    return (GXLightID)(1 << n);
}

static DataNode DebugToggleShadows(DataArray *array) {
    WiiEnviron::mbEnableShadows = 0;
    return 0;
}

static DataNode DebugToggleShowShadowTextureOnScreen(DataArray *array) {
    WiiEnviron::mbShowShadowTextureOnScreen = !WiiEnviron::mbShowShadowTextureOnScreen;
    return 0;
}

WiiEnviron::WiiEnviron() {
    if (mShadowMat == nullptr) {
        mShadowMat = New<RndMat>();
        mShadowMat->SetBlend(RndMat::kBlendSrc);
        mShadowMat->SetZMode(RndMat::kZModeDisable);
        mShadowMat->SetTexWrap(kTexWrapClamp);
        mShadowMat->SetDiffuseTex(NULL);
        mShadowMat->SetAlpha(1);
#ifdef VERSION_SZBE69_B8
        RndMat *shadowMat = mShadowMat;
        shadowMat->mColor.Set(0, 0, 0);
        shadowMat->mDirty |= 1;
#else
        mShadowMat->SetColor(Hmx::Color(0, 0, 0, 1));
#endif
        mShadowMat->SetUseEnv(false);
    }
    if (mShadowCam == nullptr) {
        mShadowCam = New<RndCam>();
    }
#ifdef MILO_DEBUG
    DataRegisterFunc("toggle_shadows", DebugToggleShadows);
    DataRegisterFunc("toggle_show_shadow_texture", DebugToggleShowShadowTextureOnScreen);
#endif
}

WiiEnviron::~WiiEnviron() {}

bool WiiEnviron::SetLight(int i, WiiLight *lit) {
    GXLightID id = LightId(i);
    lit->Update(id);
#ifdef VERSION_SZBE69_B8
    unk_0x19E = unk_0x19E | id;
#else
    unk_0x19E |= id;
#endif
    if (unk_0x19C && lit->GetType() != RndLight::kDirectional) {
        unk_0x19C = false;
    }
    return 1;
}

#ifdef VERSION_SZBE69_B8
void WiiEnviron::ClearLights() {
    unk_0x19E = 0;
    unk_0x19C = true;
}

void WiiEnviron::SetDirLight(int index, GXColor color, const Vector3 &direction) {
    GXLightObj light;
    GXLightID id = LightId(index);
    unk_0x19E = unk_0x19E | id;
    GXInitLightPos(
        &light, 1.0e18f * direction.x, 1.0e18f * direction.y, 1.0e18f * direction.z
    );
    GXInitLightDir(&light, -direction.x, -direction.y, -direction.z);
    GXInitLightAttn(&light, 1, 0, 0, 1, 0, 0);
    GXInitLightColor(&light, color);
    GXLoadLightObjImm(&light, id);
}

void WiiEnviron::Select(const Vector3 *position) {
    RndEnviron::Select(position);
    ClearLights();
    if (TheRnd->DrawMode() != kDrawShadowColor && TheRnd->DrawMode() != kDrawShadowDepth
        && TheRnd->DrawMode() != (Mode)5 && TheRnd->DrawMode() != kDrawExtrude) {
        for (ObjPtrList<RndLight>::iterator it = mLightsReal.begin();
             it != mLightsReal.end();
             ++it) {
            WiiLight *light = (WiiLight *)*it;
            if (light->Showing() && light->GetType() == RndLight::kPoint) {
                SetLight(mNumLightsReal, light);
                mNumLightsReal++;
                mNumLightsPoint++;
            }
            if (mNumLightsReal >= 2)
                break;
        }
        UpdateApproxLighting(position, 0);
    }
    TheNgStats->counters[8] += mNumLightsReal;
    TheNgStats->counters[9] += mNumLightsApprox;
}

void WiiEnviron::ApplyApproxLighting(const GXColor *colors) {
    if (GetUseApprox() && mNumLightsApprox) {
        int used = 0;
        for (uint i = 0; i < 6; ++i) {
            Vector3 direction;
            Vector3 axis = BoxMapLighting::sAxisDir[i];
            const Hmx::Matrix3 &view = ((WiiCam *)RndCam::Current())->mWiiViewXfm.m;
            Multiply(axis, view, direction);
            if (direction.z >= 0) {
                GXColor color = colors[i];
                SetDirLight(mNumLightsReal + used++, color, direction);
            }
        }
    }
}
#endif

void WiiEnviron::RenderCharactersToShadowBuffers() {
    mShadowedCharacters.clear();
    mShadowLightSet = false;
}
