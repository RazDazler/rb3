#ifndef RNDWII_RND_H
#define RNDWII_RND_H

#include "math/Geo.h"
#include "revolution/mtx/mtx.h"
#include "rndobj/Rnd.h"
#include "rndwii/SplitPostProc.h"
#include <vector>
#ifdef VERSION_SZBE69_B8
#include "os/HomeMenu_Wii.h"
#include "os/VirtualKeyboard.h"
#include "os/DiscErrorMgr_Wii.h"
#include "revolution/gx/GXFrameBuf.h"
#include "revolution/os/OSFont.h"
#endif

#ifdef VERSION_SZBE69_B8
struct NgStats {
    int counters[14];
};
extern NgStats *TheNgStats;
#endif

class WiiTex;

class WiiOrthoProj {
public:
    WiiOrthoProj();
    ~WiiOrthoProj();
    float proj[8];
};

class WiiRnd : public Rnd
#ifdef VERSION_SZBE69_B8
    ,
               public HomeMenu::Callback,
               public VirtualKeyboard::Callback,
               public DiscErrorMgrWii::Callback
#endif
{
public:
    enum SharedTexType {
#ifdef VERSION_SZBE69_B8
        kSharedTexTotal = 9
#endif
    };

    WiiRnd();
    virtual ~WiiRnd();
    virtual DataNode Handle(DataArray *, bool);
    virtual void SetAspect(Aspect a) { mAspect = a; }
    virtual void RemovePointTest(RndFlare *);
    virtual void DoPostProcess();

    void SwapFrameBuffer();
    void SetTriFrameRendering(bool);
    void SetOrthoProj();
    void DoPointTests();
    bool GetProgressiveScan();
    void CopyBuffer();
#ifdef VERSION_SZBE69_B8
    void PrepareRenderAlley();
    void ResetAfterHang();
#endif
    void DrawQuad(const Hmx::Rect &);
    void DrawQuad(int, int);
    void DrawLine(const Vector3 &, const Vector3 &, const Hmx::Color &, bool);
    void WiiPreInit();
    void SetFullScrProj();
    void PreInit();
    void ClearSwapTables();
    void DrawBlackBackground();
#ifdef VERSION_SZBE69_B8
    virtual void BeginDrawing();
    void BeginDrawing(bool);
    virtual void EndDrawing();
    void GXReInit(bool);
    bool Ntsc();
    virtual bool CanModal(bool);
    virtual void SetSync(int);
    virtual float YRatio();
    virtual bool GetOverscan() const;
    virtual void SetOverscan(bool);
    RndTex *PostProcessTexture();
    virtual RndTex *GetCurrentFrameTex(bool);
    virtual void DrawPreClear();
    virtual void DoWorldEnd();
    virtual void ClearBuffer();
    virtual void HomeMenuOpen(bool);
    virtual void HomeMenuClose(bool);
    virtual void HomeMenuDraw();
    virtual void HomeMenuBannedDraw(short, void *);
    virtual void SDIconDraw(short, void *);
#endif
    void KeyboardOpen();
    void KeyboardClose();
#ifdef VERSION_SZBE69_B8
    virtual void DiscErrorStart();
    virtual void DiscErrorDraw(void *);
    virtual void DiscErrorEnd();
    virtual void ResetStats();
#endif
    RndTex *GetSharedTex(SharedTexType, bool);
#ifdef VERSION_SZBE69_B8
    static WiiTex *mSharedTexture[3];
#endif

#ifdef VERSION_SZBE69_B8
    // Callback vptrs occupy 0x160, 0x164 and 0x168.
    GXRenderModeObj mRenderMode; // 0x16c..0x1a7
    void *unk_0x1A8, *unk_0x1AC;
    bool unk_0x1B0;
    void *unk_0x1B4;
    u32 unk_0x1B8;
    Mtx44 unk_0x1BC, unk_0x1FC;
    Mtx unk_0x23C; // 3x4 position matrix, 0x23c..0x26b
    OSFontHeader *mFontHeader; // 0x26c
    bool unk_0x270;
    WiiSplitPostProc *unk_0x274;
    float mSavedProjection[7]; // GXGetProjectionv, 0x278
    float mSavedViewport[6]; // GXGetViewportv, 0x294
    bool mProgScan; // 0x2ac
    bool unk_0x2AD, unk_0x2AE, unk_0x2AF;
    bool unk_0x2B0, unk_0x2B1, unk_0x2B2, unk_0x2B3;
    std::vector<Rnd::PointTest> unk_0x2B4;
    bool unk_0x2BC;
    int mFramesBuffered; // 0x2c0
#else
    ushort unk_0x170, unk_0x172;
    u8 unk_0x184, unk_0x185;
    void *unk_0x1A8, *unk_0x1AC, *unk_0x1B0, *unk_0x1B4;
    u32 unk_0x1B8;
    Mtx44 unk_0x1BC, unk_0x1FC, unk_0x23C;
    WiiSplitPostProc *unk_0x274;
    bool mProgScan; // 0x2AC
    bool unk_0x2B0, unk_0x2B1, unk_0x2B2, unk_0x2B3;
    std::vector<Rnd::PointTest> unk_0x2B4;
    bool unk_0x2BC;
    int mFramesBuffered; // 0x2C0
#endif

    ushort FrameBufferWidth() const {
#ifdef VERSION_SZBE69_B8
        return mRenderMode.fbWidth;
#else
        return unk_0x170;
#endif
    }
    ushort EmbeddedFrameBufferHeight() const {
#ifdef VERSION_SZBE69_B8
        return mRenderMode.efbHeight;
#else
        return unk_0x172;
#endif
    }
    u8 AntialiasingEnabled() const {
#ifdef VERSION_SZBE69_B8
        return mRenderMode.aa;
#else
        return unk_0x185;
#endif
    }

    static bool mUseLockedCache, mShowParticle, mShowAssetName;
    static void ToggleAssetName() { mShowAssetName = !mShowAssetName; }
    static void ToggleShowParticle() { mShowParticle = !mShowParticle; }
    static void ToggleLockedCache() { mUseLockedCache = !mUseLockedCache; }
    static void SyncFree(void *);
#ifdef VERSION_SZBE69_B8
    void SyncDestroy();
#endif
    static void *GetCurrXFB();
};

void SetGPHangDetectEnabled(bool, const char *);
void RndGXBegin(_GXPrimitive prim, _GXVtxFmt fmt, unsigned short verts);
void RndGXEnd();
void RndGxDrawDone();
void MakeWiiMtxTex(const Transform &, bool, Mtx);
void MakeWiiMtx(const Transform &, Mtx &);
#ifdef VERSION_SZBE69_B8
inline uint MakeU32Color(const Hmx::Color &color) {
    uint packed;
    register const Hmx::Color *src = &color;
    register u8 *dst = (u8 *)&packed;
    register __vec2x32float__ ba, rg;
    // clang-format off
    ASM_BLOCK {
        psq_l ba, 8(src), 0, 0
        psq_l rg, 0(src), 0, 0
        psq_st rg, 0(dst), 0, 6
        psq_st ba, 2(dst), 0, 6
    }
    // clang-format on
    return packed;
}
#else
uint MakeU32Color(const Hmx::Color &);
#endif

extern WiiRnd TheWiiRnd;
extern int gSuppressPointTest;
extern bool gbDbgRequestForcedHang;
extern bool gbDbgRequestHangRecovery;
extern bool gRecoveringThisFrame;

#endif // RNDWII_RND_H
