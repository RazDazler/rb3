#include "Rnd.h"
#include "decomp.h"

#if defined(VERSION_SZBE69_B8)
// B8 literal order precedes the retained retail metadata helper.
DECOMP_FORCEACTIVE(
    HighB8LiteralPoolRnd,
    "",
    "Rnd.cpp",
    "gGPHangDetectDisabled >= 0",
    "AnonFunc",
    "Fifo Status\tbase:%p\tsize:\t%d\tlow:\t%d\thi:\t%d\tlinked:\t%d\n",
    "readPtr\twritePtr\treadIndex\twriteIndex\tcount\twrap\n",
    "%p\t%p\t%d\t%d\t%d\t%d\n",
    "Something bad happened\n",
    "MemErrorHandler: dsisr: %d dar: %p\n",
    "OSResumeThread returned %d\n",
    "unnamed",
    "The thread called \"%s\"\nHas just run out of stack space\nThis is bad, real bad\nmagic %d\nsp %08x\nbase %08x\nend %08x\npad %d\nlr %08x\n",
    "GPHangDebug: forcing gp hang recovery.\n",
    "GP Hang Detection disabled from %s - ignoring %d missed frames\n",
    "gs_regular",
    "gs_char",
    "gs_postproc",
    "gSuppressPointTest>=0",
    "gSuppressPointTest>0",
    "!gSuppressPointTest",
    "ResetAfterHang: calling OnGPHangRecover() for post procs.\n",
    "ResetAfterHang: resetting shared textures.\n",
    "ResetAfterHang: clearing delayed free lists\n",
    "  GXGetGPStatus(%d, %d, %d, %d, %d);\n",
    "  GX FIFO overflow count: %d\n",
    "GXNtsc480ProgSoft\n",
    "GXNtsc480IntDf\n",
    "main",
    "fast",
    "GXSetPixelFmt( GX_PF_RGB565_Z16, GX_ZC_LINEAR );\n",
    "GXSetPixelFmt(GX_PF_RGB565_Z16, GX_ZC_LINEAR);\n",
    "cpu",
    "hang detected, resetting\n",
    "hang detected2, resetting\n",
    "world_gs",
    "gs",
    "sFreeFrame",
    "s",
    "%s(%d): %s unhandled msg: %s",
    "GXSetPixelFmt(GX_PF_RGB8_Z24, GX_ZC_LINEAR);\n",
    "faces %d %d\n",
    "parts %d %d\n",
    "part_sys %d %d\n",
    "reg_meshes %d %d\n",
    "mut_meshes %d %d\n",
    "bones %d %d\n",
    "mats %d %d\n",
    "cams %d %d\n",
    "lights %d %d\n",
    "multimesh %d %d\n",
    "faces %d\n",
    "parts %d\n",
    "part_sys %d\n",
    "reg_meshes %d\n",
    "mut_meshes %d\n",
    "bones %d\n",
    "mats %d\n",
    "cams %d\n",
    "lights %d\n",
    "multimesh %d\n",
    "----------------------------------\n",
    "%f %f %f %f\n",
    "%f %f %f %f\n\n",
    "%f\n",
    "%d\n",
    "0 <= type && type < kSharedTexTotal",
    "!gInBegin",
    "vector"
)
#endif
#include "obj/Object.h"
#include "os/Debug.h"
#include "os/PlatformMgr.h"
#include "revolution/gx/GXAttr.h"
#include "revolution/gx/GXFifo.h"
#include "revolution/gx/GXFrameBuf.h"
#include "revolution/gx/GXLight.h"
#include "revolution/gx/GXPixel.h"
#include "revolution/gx/GXTev.h"
#include "revolution/gx/GXTransform.h"
#include "revolution/gx/GXTypes.h"
#include "revolution/gx/GXVert.h"
#include "revolution/os/OSError.h"
#include "revolution/sc/scapi.h"
#include "revolution/vi/vi.h"
#include "rndobj/Flare.h"
#include "rndobj/Font.h"
#include "rndobj/HiResScreen.h"
#include "rndobj/Rnd.h"
#ifdef VERSION_SZBE69_B8
#include "rndobj/Utl.h"
#endif
#ifdef VERSION_SZBE69_B8
// Retain the original renderer Light class-name metadata in this unit only.
#define RB3_B8_FORCE_WIILIGHT_CLASSNAME
#include "rndwii/Env.h"
#include "rndwii/Mesh.h"
// Static API recovered; the particle class and translation unit remain incomplete.
class WiiParticleSys {
public:
    static void Init();
};
#endif
#ifdef VERSION_SZBE69_B8
#include "rndwii/Lit.h"
#endif
#include "rndwii/Mat.h"
#include "rndwii/Tex.h"
#include "types.h"
#include "utl/MemMgr.h"
#include "utl/Symbols.h"
#include "revolution/GX.h"
#include "revolution/OS.h"
#include <cstdarg>

bool gInBegin, gBeginIntState;
int gSuppressPointTest;
void *sDispFB, *sCopyFB;
OSThreadQueue drawDoneThreadQueue;
WiiRnd TheWiiRnd;

#ifdef VERSION_SZBE69_B8
#include <list>
#include "os/Timer.h"

volatile int gFrameCount;
volatile int gGPHangDetectDisabled;
const char *gpLastHangDetectDisabler = "";
void SetGPHangDetectEnabled(bool enabled, const char *caller) {
    if (enabled) {
        if (gGPHangDetectDisabled == 1) {
            gFrameCount = 0;
            gpLastHangDetectDisabler = "";
        }
        gGPHangDetectDisabled--;
        MILO_ASSERT(gGPHangDetectDisabled >= 0, 76);
    } else {
        if (caller && *caller)
            gpLastHangDetectDisabler = caller;
        else
            gpLastHangDetectDisabler = "AnonFunc";
        gGPHangDetectDisabled++;
    }
}
std::list<void *> sDelayedFreeLists[4];
int sDelayedFreeListIndex;
extern OSThreadQueue sThreadQueue, netThreadQueue;
extern Timer gTriFrameTimer;
struct WiiFrame {
    void *mFrameBuffer;
    void *mBreakpoint;
    u32 unk8;
    WiiFrame *mNext;
};
WiiFrame *sCurrentFrame;
int gLastQueueSize;
Timer sFrameTimer;

int FrameQueueSize() {
    int count = 0;
    for (WiiFrame *frame = sCurrentFrame; frame; frame = frame->mNext)
        ++count;
    gLastQueueSize = count;
    return count;
}

void StopRenderingFrame() { sFrameTimer.Stop(); }

void WiiRnd::SyncFree(void *p) {
    if (p)
        sDelayedFreeLists[sDelayedFreeListIndex].push_back(p);
}

void WiiRnd::SyncDestroy() {
    sDelayedFreeListIndex = (sDelayedFreeListIndex + 1) % 4;
    for (std::list<void *>::iterator it =
             sDelayedFreeLists[sDelayedFreeListIndex].begin();
         it != sDelayedFreeLists[sDelayedFreeListIndex].end();
         ++it) {
        _MemFree(*it);
    }
    sDelayedFreeLists[sDelayedFreeListIndex].clear();
}

void FrameQueueSync(int bufferedFrames) {
    OSWakeupThread(&netThreadQueue);
    gTriFrameTimer.Stop();
    BOOL interrupts = OSDisableInterrupts();
    if (FrameQueueSize() > bufferedFrames - 1)
        OSSleepThread(&sThreadQueue);
    OSRestoreInterrupts(interrupts);
    gTriFrameTimer.Restart();
}

void WiiRnd::HomeMenuOpen(bool) {
    SetGPHangDetectEnabled(false, __FUNCTION__);
    unk_0x2B2 = GetOverscan();
    SetOverscan(false);
}
#endif

void DumpFifoStatus() {
    GXFifoObj fifo;
    GXGetCPUFifo(&fifo);
    GXGetFifoCount(&fifo);
    GXGetFifoWrap(&fifo);

    OSReport("Fifo Status\tbase:%p\tsize:\t%d\tlow:\t%d\thi:\t%d\tlinked:\t%d\n");
    OSReport("readPtr\twritePtr\treadIndex\twriteIndex\tcount\twrap\n");
    OSReport("%p\t%p\t%d\t%d\t%d\t%d\n");
}

#ifdef VERSION_SZBE69_B8
extern "C" void MemErrorHandler(u8 error, OSContext *ctx, u32 dsisr, u32 dar) {
    OSReport("MemErrorHandler: dsisr: %d dar: %p\n", dsisr, dar | 0x80000000);
}
#else
extern "C" void MemErrorHandler(u8 error, OSContext *ctx, u32 dsisr, u32 dar, ...) {
    OSReport("MemErrorHandler: dsisr: %d dar: %p\n", dsisr, dar);
}
#endif

void WiiRnd::SwapFrameBuffer() {
    if (sDispFB == unk_0x1A8)
        sDispFB = unk_0x1AC;
    else
        sDispFB = unk_0x1A8;
}

void WiiRnd::RemovePointTest(RndFlare *) {
    if (!TheHiResScreen.mActive) {
        MILO_ASSERT(gSuppressPointTest>=0, 876);

        MILO_ASSERT(gSuppressPointTest>0, 897);
        gSuppressPointTest--;
    }
}

#ifdef VERSION_SZBE69_B8
void WiiRnd::DoPointTests() {
    if (!unk_0x2BC) {
        unk_0x2BC = true;
        for (std::vector<Rnd::PointTest>::iterator it = unk_0x2B4.begin();
             it != unk_0x2B4.end();
             ++it) {
            if (it->unk_0xC->mPointTest) {
                u32 depth;
                GXPeekZ(it->unk_0x0, it->unk_0x4, &depth);
                it->unk_0xC->SetVisible(depth > it->unk_0x8);
            }
            if (it->unk_0xC->mAreaTest) {
                RndFlare *flare = it->unk_0xC;
                float area = (int)(flare->mArea.w * flare->mArea.h);
                flare->SetTestDone();
                flare->unkec = area;
            }
        }
    }
}
#endif

#ifdef VERSION_SZBE69_B8
WiiFrame *sFreeFrame;
WiiFrame *sRenderingFrame;
bool sBPSet, sBPGo, sBPWait;
int gShouldDiagnose;
bool gDebugResetAfterHang;
void StartRenderingFrame() {
    sFrameTimer.Start();
    if (!sCurrentFrame) {
        OSReport("Something bad happened\n");
    } else {
        sRenderingFrame = sCurrentFrame;
        GXEnableBreakPt(sCurrentFrame->mBreakpoint);
    }
}
void SetNextGXBreakpoint() {
    if (sCurrentFrame) {
        WiiFrame *frame = sCurrentFrame;
        sCurrentFrame = frame->mNext;
        frame->mNext = sFreeFrame;
        sFreeFrame = frame;
        OSWakeupThread(&sThreadQueue);
        if (!sCurrentFrame) {
            GXDisableBreakPt();
            sBPSet = false;
        } else {
            StartRenderingFrame();
        }
    } else {
        OSWakeupThread(&sThreadQueue);
        GXDisableBreakPt();
    }
}
void DiagnoseHang();
void OnVIPostRetrace(u32) {
    if (sBPWait && sBPGo) {
        sBPGo = false;
        sBPWait = false;
        SetNextGXBreakpoint();
    } else if (gShouldDiagnose) {
        DiagnoseHang();
        gShouldDiagnose = 0;
    } else if (gDebugResetAfterHang) {
        gDebugResetAfterHang = false;
        TheWiiRnd.ResetAfterHang();
        GXFlush();
    }
}
#endif

void OnDrawSync(u16 s) {
    if (s != 62001)
        return;
    if (gSuppressPointTest)
        return;
    TheWiiRnd.DoPointTests();
}

#ifdef VERSION_SZBE69_B8
void WiiRnd::GXReInit(bool waitForRetrace) {
    if (AntialiasingEnabled()) {
        GXSetPixelFmt(GX_PF_RGBA565_Z16, GX_ZC_LINEAR);
        MILO_LOG("GXSetPixelFmt(GX_PF_RGB565_Z16, GX_ZC_LINEAR);\n");
    } else {
        GXSetPixelFmt(GX_PF_RGB8_Z24, GX_ZC_LINEAR);
    }
    GXSetDither(true);
    uint copyHeight = GXSetDispCopyYScale(
        GXGetYScaleFactor(mRenderMode.efbHeight, mRenderMode.xfbHeight)
    );
    GXSetTevSwapModeTable(
        GX_TEV_SWAP1, GX_CH_GREEN, GX_CH_GREEN, GX_CH_GREEN, GX_CH_GREEN
    );
    uint clearColor = MakeU32Color(mClearColor);
    GXSetCopyClear(*(GXColor *)&clearColor, 0xFFFFFF);
    GXSetDispCopySrc(0, 0, mRenderMode.fbWidth, mRenderMode.efbHeight);
    GXSetDispCopyDst(mRenderMode.fbWidth, copyHeight);
    GXSetCopyClamp(GX_CLAMP_ALL);
    CopyBuffer();
    GXSetDispCopyGamma(0);
    if (waitForRetrace) {
        VIFlush();
        VIWaitForRetrace();
        VIWaitForRetrace();
    }
    SetSync(mSync);
    GXSetMisc(1, 8);
    C_MTXOrtho(unk_0x1BC, 0, mRenderMode.efbHeight, 0, mRenderMode.fbWidth, 0, -100);
    PSMTXIdentity(unk_0x23C);
    GXSetVtxAttrFmt(GX_VTXFMT6, GX_VA_POS, GX_POS_XYZ, GX_F32, 0);
    GXSetVtxAttrFmt(GX_VTXFMT6, GX_VA_CLR0, GX_CLR_RGBA, GX_RGBA8, 0);
    GXSetVtxAttrFmt(GX_VTXFMT6, GX_VA_TEX0, GX_TEX_ST, GX_U8, 0);
    GXSetVtxAttrFmt(GX_VTXFMT7, GX_VA_POS, GX_POS_XYZ, GX_S16, 0);
    GXSetVtxAttrFmt(GX_VTXFMT7, GX_VA_TEX0, GX_TEX_ST, GX_S16, 0);
    GXSetDrawSyncCallback(OnDrawSync);
    WiiMesh::Init();
    WiiParticleSys::Init();
}
#endif

#ifdef VERSION_SZBE69_B8
bool gPreInitComplete;
bool gInitComplete;
bool gFinishFrame;
void FrameQueueSync(int);
void WiiRnd::BeginDrawing(bool fullFrame) {
    if (fullFrame) {
        if (!gPreInitComplete)
            return;
        if (!gInitComplete) {
            GXSetZMode(true, GX_LEQUAL, true);
            GXSetColorUpdate(true);
            GXSetAlphaUpdate(true);
            uint clearColor = MakeU32Color(mClearColor);
            GXSetCopyClear(*(GXColor *)&clearColor, 0xFFFFFF);
            if (unk_0x1B0) {
                VISetBlack(false);
                VIFlush();
                unk_0x1B0 = false;
            }
            if (mSync) {
                if (!gFinishFrame)
                    VIWaitForRetrace();
                gFinishFrame = false;
            }
        } else {
            {
                static Timer *_t = AutoTimer::GetTimer("cpu");
                if (_t)
                    _t->Stop();
            }
            if (unk_0x1B0) {
                VISetBlack(false);
                VIFlush();
                unk_0x1B0 = false;
            }
            FrameQueueSync(mFramesBuffered);
            SyncDestroy();
            Rnd::BeginDrawing();
            {
                static Timer *_t = AutoTimer::GetTimer("cpu");
                if (_t)
                    _t->Start();
            }
            WiiMat::sCurrent = 0;
            ClearSwapTables();
            mWorldEnded = true;
        }
    } else {
        BeginDrawing();
    }
}
void WiiRnd::HomeMenuClose(bool) {
    GXReInit(true);
    SetOverscan(unk_0x2B2);
    SetGPHangDetectEnabled(true, __FUNCTION__);
    BeginDrawing(true);
    EndDrawing();
    BeginDrawing(true);
    EndDrawing();
    BeginDrawing(true);
    EndDrawing();
}

#endif

void DiagnoseHang() {
    u8 a, b, c, d, e;
    GXGetGPStatus(&a, &b, &c, &d, &e);
    OSReport("  GXGetGPStatus(%d, %d, %d, %d, %d);\n", a, b, c, d, e);
    OSReport("  GX FIFO overflow count: %d\n", GXGetOverflowCount());
    GXResetOverflowCount();
    DumpFifoStatus();
}

WiiOrthoProj::WiiOrthoProj() {
    GXGetProjectionv(proj);
    TheWiiRnd.SetOrthoProj();
}

WiiOrthoProj::~WiiOrthoProj() { GXSetProjectionv(proj); }

WiiRnd::WiiRnd()
#ifdef VERSION_SZBE69_B8
    : unk_0x1B0(true), unk_0x1B4(0), unk_0x1B8(0), mFontHeader(0), unk_0x270(true),
      unk_0x2AD(false), unk_0x2AE(false), unk_0x2AF(true), unk_0x2B0(false),
      unk_0x2B1(false), unk_0x2B2(false), unk_0x2B3(false), unk_0x2BC(false),
      mFramesBuffered(2) {
    unk_0x1A8 = unk_0x1AC = 0;
#else
    : unk_0x2B0(false), unk_0x2B1(false), unk_0x2B2(false), unk_0x2B3(false),
      unk_0x2BC(false), mFramesBuffered(2) {
#endif
    mClearColor.Set(0, 0, 0, 0);
    unk_0x2B4.reserve(0x20);
}

WiiRnd::~WiiRnd() {}

#ifdef VERSION_SZBE69_B8
bool WiiRnd::Ntsc() { return (uint)(VIGetTvFormat() - VI_TV_FMT_PAL) > 1; }
void SetupDrawStringVertexDesc() {
    GXClearVtxDesc();
    GXSetVtxDesc(GX_VA_POS, GX_DIRECT);
    GXSetVtxDesc(GX_VA_TEX0, GX_DIRECT);
}
RndTex *WiiRnd::GetCurrentFrameTex(bool) { return PostProcessTexture(); }
void WiiRnd::DrawPreClear() {
    Rnd::DrawPreClear();
    if (TheWiiRnd.ProcCmds() & 1)
        WiiEnviron::RenderCharactersToShadowBuffers();
}
void WiiRnd::ClearBuffer() {}
void WiiRnd::DoWorldEnd() {
    if (mProcCmds & 4)
        GXSetDrawSync(0xF231);
    ClearSwapTables();
    Rnd::DoWorldEnd();
}
bool WiiRnd::CanModal(bool) { return true; }
void WiiRnd::SetSync(int sync) { mSync = sync; }
float WiiRnd::YRatio() {
#ifdef VERSION_SZBE69_B8
    if (mAspect == kWidescreen)
        return 0.5625f;
    if (mAspect == kLetterbox)
        return 0.5625f;
    return 0.75f;
#else
    if (mAspect == kWidescreen || mAspect == kLetterbox)
        return 0.5625f;
    else
        return 0.75f;
#endif
}
bool WiiRnd::GetOverscan() const { return unk_0x2B1; }
void WiiRnd::SetOverscan(bool enabled) { unk_0x2B1 = enabled; }
void WiiRnd::SetTriFrameRendering(bool enabled) {
    if ((uint)VIGetTvFormat() != VI_TV_FMT_PAL || mProgScan)
        mProcCounter.mTriFrameRendering = enabled;
}
void WiiRnd::DiscErrorStart() {
    ThePlatformMgr.SetScreenSaver(true);
    RndSplasherSuspend();
}
void WiiRnd::DiscErrorEnd() { RndSplasherResume(); }
RndTex *WiiRnd::PostProcessTexture() {
    if (unk_0x274)
        return unk_0x274->mTex;
    else
        return 0;
}
#endif

void WiiModal(bool &rb, char *c, bool b) { TheWiiRnd.Modal(rb, c, b); }

void WiiRnd::PreInit() {
    if (!unk_0x2B3) {
        unk_0x2B3 = true;
        WiiPreInit();
#ifdef VERSION_SZBE69_B8
        OSSetErrorHandler(15, (OSErrorHandler)MemErrorHandler);
#else
        OSSetErrorHandler(15, MemErrorHandler);
#endif
        if (unk_0x1A8 != nullptr) {
            _MemFree(unk_0x1A8);
            unk_0x1A8 = nullptr;
        }
        if (unk_0x1AC != nullptr) {
            _MemFree(unk_0x1AC);
            unk_0x1AC = nullptr;
        }
        if (unk_0x1B4 != nullptr) {
            _MemFree(unk_0x1B4);
            unk_0x1B4 = nullptr;
        }
        unk_0x1B8 = 0;
        ThePlatformMgr.InitGQR();
        VIInit();
        VITvFormat fmt = VIGetTvFormat();
        mProgScan = VIGetScanMode() == VI_SCAN_MODE_PROG;
        if (SCGetAspectRatio() == 1) {
            SetAspect(kWidescreen);
        } else {
            SetAspect(kLetterbox);
        }
        if (fmt == VI_TV_FMT_NTSC) {
        } else if (fmt == VI_TV_FMT_PAL) {
        } else if (fmt == VI_TV_FMT_EURGB60) {
            mProgScan;
        }
    }
}

void WiiRnd::WiiPreInit() {
    Rnd::PreInit();
    WiiTex::Register();
    WiiMat::PreInit();
}

bool WiiRnd::GetProgressiveScan() { return mProgScan; }

void WiiRnd::DrawLine(const Vector3 &a, const Vector3 &b, const Hmx::Color &c, bool) {
    GXLoadPosMtxImm(unk_0x23C, 0);
    GXSetCurrentMtx(0);
    GXClearVtxDesc();
    GXSetVtxDesc(GX_VA_POS, GX_DIRECT);
    GXSetVtxDesc(GX_VA_CLR0, GX_DIRECT);
    int col = MakeU32Color(c);
    RndGXBegin(GX_LINES, GX_VTXFMT6, 2);
    GXPosition3f32(a.x, a.y, a.z);
    GXColor1u32(col);
    GXPosition3f32(b.x, b.y, b.z);
    GXColor1u32(col);
    RndGXEnd();
}

void WiiRnd::DoPostProcess() {
    Rnd::DoPostProcess();
    unk_0x2B0 = false;
    unk_0x274->DrawFinalTex();
    WiiMat::sCurrent = nullptr;
}

BEGIN_HANDLERS(WiiRnd)
    HANDLE_ACTION(tri_frame, SetTriFrameRendering(_msg->Int(2)))
    HANDLE_ACTION(toggle_locked_cache, ToggleLockedCache())
    HANDLE_ACTION(toggle_show_particle, ToggleShowParticle())
    HANDLE_ACTION(toggle_asset_name, ToggleAssetName())
    HANDLE_ACTION(frames_buffered, mFramesBuffered = _msg->Int(2))
    HANDLE_SUPERCLASS(Rnd)
    HANDLE_CHECK(2722)
END_HANDLERS

#ifdef VERSION_SZBE69_B8
void WiiRnd::PrepareRenderAlley() {
    WiiTex *first = (WiiTex *)GetSharedTex((SharedTexType)3, true);
    WiiTex *second = (WiiTex *)GetSharedTex((SharedTexType)4, true);
    first->CopyContent(false);
    second->CopyContent(true);
    GXSetPixelFmt(GX_PF_RGBA6_Z24, GX_ZC_LINEAR);
}
#endif

#ifdef VERSION_SZBE69_B8
struct SharedTexInfo {
    int index, width, height, bpp;
    u32 format, type;
    bool mipmap;
};
extern const SharedTexInfo gSharedTexInfo[9] = {
    { 0, 608, 456, 16, 6, 2, true },    { 1, 608, 456, 24, 22, 162, true },
    { 2, 128, 256, 16, 6, 2, true },    { 0, 128, 256, 16, 6, 2, true },
    { 1, 128, 256, 24, 22, 162, true }, { 2, 128, 256, 16, 3, 2, false },
    { 0, 304, 228, 16, 6, 2, true },    { 1, 304, 228, 16, 19, 162, true },
    { 0, 608, 456, 8, 1, 2, true },
};
WiiTex *WiiRnd::mSharedTexture[3];
RndTex *WiiRnd::GetSharedTex(SharedTexType type, bool resize) {
    MILO_ASSERT(0 <= type && type < kSharedTexTotal, 3660);
    int index = gSharedTexInfo[type].index;
    if (resize) {
        mSharedTexture[index]->Resize(
            gSharedTexInfo[type].width,
            gSharedTexInfo[type].height,
            (GXTexFmt)gSharedTexInfo[type].format,
            gSharedTexInfo[type].mipmap
        );
    }
    return mSharedTexture[index];
}
#endif

#ifdef VERSION_SZBE69_B8
// B8 has three statistics records, each containing fourteen 32-bit counters.
// ResetStats establishes the size; indices 7 and 8 are seeded each frame.
NgStats gNgStats[3];
NgStats *TheNgStats = gNgStats;
void WiiRnd::ResetStats() {
    if (mProcCmds == 1 || mProcCmds == 7) {
        TheNgStats = &gNgStats[0];
    } else {
        TheNgStats = mProcCmds == 4 ? &gNgStats[1] : &gNgStats[2];
    }
    memset(TheNgStats, 0, sizeof(NgStats));
    TheNgStats->counters[7]++;
    TheNgStats->counters[8]++;
}
#endif

void WiiRnd::CopyBuffer() {
#ifdef VERSION_SZBE69_B8
    GXSetCopyFilter(mRenderMode.aa, mRenderMode.sample_pattern, true, mRenderMode.vfilter);
#else
    GXSetCopyFilter(AntialiasingEnabled(), 0, true, 0);
#endif
    GXCopyDisp(sCopyFB, true);
    GXSetCopyFilter(0, 0, 0, 0);
}

void WiiRnd::SetOrthoProj() {
    if (unk_0x2B0)
        GXSetProjection(unk_0x1FC, GX_ORTHOGRAPHIC);
    else
        GXSetProjection(unk_0x1BC, GX_ORTHOGRAPHIC);
    GXLoadPosMtxImm(unk_0x23C, 0);
    GXSetCurrentMtx(0);
}

void WiiRnd::DrawQuad(int w, int h) {
    Hmx::Rect r;
    if (w == 0) {
        if (!unk_0x2B0)
            w = mWidth;
        else
            w = 608;
    }
    if (h == 0)
        h = mHeight;
    r.Set(0, 0, w, h);

    DrawQuad(r);
}

void WiiRnd::DrawQuad(const Hmx::Rect &r) {
    GXClearVtxDesc();
    GXSetVtxDesc(GX_VA_POS, GX_DIRECT);
    GXSetVtxDesc(GX_VA_TEX0, GX_DIRECT);
    RndGXBegin(GX_QUADS, GX_VTXFMT6, 4);
    float rx = r.x;
    float ry = r.y;
    float rh = r.h;
    GXPosition3f32(rx, ry, 0);
    GXTexCoord2u8(0, 0);
    GXPosition3f32(rx, rh, 0);
    GXTexCoord2u8(0, 1);
    GXPosition3f32(r.w, rh, 0);
    GXTexCoord2u8(1, 1);
    GXPosition3f32(r.w, ry, 0);
    GXTexCoord2u8(1, 0);
    RndGXEnd();
}

void WiiRnd::ClearSwapTables() {
    int i = 0;
    do {
        GXSetTevSwapMode((GXTevStageID)i, GX_TEV_SWAP0, GX_TEV_SWAP0);
    } while (++i < 16);
}

void WiiRnd::DrawBlackBackground() {
    GXColor c;
    c.b = c.g = c.r = 0;
    c.a = 0xff;
    GXColor c2;
    c.b = c.g = c.r = 0;
    c.a = 0xff;
    GXClearVtxDesc();
    GXSetVtxDesc(GX_VA_POS, GX_DIRECT);
    GXSetVtxDesc(GX_VA_CLR0, GX_DIRECT);
    GXSetChanAmbColor(GX_COLOR0A0, c2);
    GXSetChanMatColor(GX_COLOR0A0, c);
    GXSetChanCtrl(
        GX_COLOR0A0, false, GX_SRC_REG, GX_SRC_REG, GX_LIGHT_NULL, GX_DF_CLAMP, GX_AF_NONE
    );
    GXSetChanCtrl(
        GX_COLOR1A1, false, GX_SRC_REG, GX_SRC_VTX, GX_LIGHT_NULL, GX_DF_NONE, GX_AF_NONE
    );
    GXSetNumTexGens(0);
    GXSetNumChans(1);
    GXSetNumTevStages(1);
    GXSetTevOrder(GX_TEVSTAGE0, GX_TEXCOORD_NULL, GX_TEXMAP_NULL, GX_COLOR0A0);
    GXSetTevColorIn(GX_TEVSTAGE0, GX_CC_ZERO, GX_CC_ZERO, GX_CC_ZERO, GX_CC_RASC);
    GXSetTevAlphaIn(GX_TEVSTAGE0, GX_CA_ZERO, GX_CA_ZERO, GX_CA_ZERO, GX_CA_RASA);
    GXSetTevColorOp(GX_TEVSTAGE0, GX_TEV_ADD, GX_TB_ZERO, GX_CS_SCALE_1, 1, GX_TEVPREV);
    GXSetTevAlphaOp(GX_TEVSTAGE0, GX_TEV_ADD, GX_TB_ZERO, GX_CS_SCALE_1, 1, GX_TEVPREV);
    RndGXBegin(GX_QUADS, GX_VTXFMT1, 4);
    GXPosition3f32(0, 0, 0);
    GXColor4u8(0, 0, 0, 0xff);
    GXPosition3f32(0, FrameBufferWidth(), 0);
    GXColor4u8(0, 0, 0, 0xff);
    GXPosition3f32(EmbeddedFrameBufferHeight(), FrameBufferWidth(), 0);
    GXColor4u8(0, 0, 0, 0xff);
    GXPosition3f32(EmbeddedFrameBufferHeight(), 0, 0);
    GXColor4u8(0, 0, 0, 0xff);
    RndGXEnd();
}

void WiiRnd::KeyboardOpen() { SetGPHangDetectEnabled(false, __FUNCTION__); }

void WiiRnd::KeyboardClose() {
    SetGPHangDetectEnabled(true, __FUNCTION__);
    BeginDrawing();
    EndDrawing();
    BeginDrawing();
    EndDrawing();
    BeginDrawing();
    EndDrawing();
}

void *WiiRnd::GetCurrXFB() { return sDispFB; }

void RndGXBegin(_GXPrimitive prim, _GXVtxFmt fmt, unsigned short verts) {
    MILO_ASSERT(!gInBegin, 3699);
    gInBegin = true;
    gBeginIntState = OSDisableInterrupts();
    GXBegin(prim, fmt, verts);
}

void RndGXEnd() { // this never calls GXEnd but it's ok because it's a nothing burger
    OSRestoreInterrupts(gBeginIntState);
    gInBegin = false;
}

void RndGXDrawDoneCallback() { OSWakeupThread(&drawDoneThreadQueue); }

void RndGxDrawDone() {
    int x = OSDisableInterrupts();
    GXSetDrawDoneCallback(RndGXDrawDoneCallback);
    GXSetDrawDone();
    OSSleepThread(&drawDoneThreadQueue);
    OSRestoreInterrupts(x);
}
