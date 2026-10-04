#include "Tex.h"
#include "decomp.h"

#if defined(VERSION_SZBE69_B8)
// Preserve original literal ordering while incomplete methods remain.
DECOMP_FORCEACTIVE(
    LiteralPoolTex,
    "Currently unsupported format %d for OrderFromFormat.\n",
    "%s: too large for a render target, width %d (max %d)\n",
    "%s: too large for a render target, height %d (max %d)\n",
    "Tex.cpp",
    "!mNumMips",
    "mBitmap.Order() & RndBitmap::kDXT1",
    "!mImageData",
    "!mPaletteData",
    "%s: mipmapped textures dimensions must be a power of 2\n",
    "!((int)mImageData & 31)",
    "!mAlphaImageData",
    "!((int)mAlphaImageData & 31)",
    "!mMovieImage[0]",
    "!mMovieImage[1]",
    "Locking of Wii textures with alpha not currently supported!\n",
    "mImageData 0x%08x being leaked!\n",
    "mImageData",
    "mType & kRendered",
    "[ERROR] Trying to compress a compressed texture!\n",
    "0",
    "[WiiTex::CreateScreenShot] Failed to covert XFB to BMP!\n"
)
#endif
#include "os/Debug.h"
#include "revolution/gx/GXFrameBuf.h"
#include "revolution/gx/GXPixel.h"
#ifdef VERSION_SZBE69_B8
#include "revolution/gx/GXMisc.h"
#include "revolution/os/OSCache.h"
#endif
#include "rndobj/Rnd.h"
#include "rndobj/Tex.h"
#include "rndwii/Rnd.h"
#include <set>
#ifdef VERSION_SZBE69_B8
#include <stdlib.h>
#include "utl/MemMgr.h"
// The B8 texture unit calls the out-of-line endian specializations.
template <class T>
void EndianSwapEq(T &);
#include "rndobj/Dxt1Compress.h"
#include <string.h>
#endif

std::set<WiiTex *> gRenderTextureSet;

bool WiiTex::bComposingOutfitTexture = false;

#ifdef VERSION_SZBE69_B8
WiiTex::WiiTex()
    : mImageData(NULL), mFormat(), unk_0xAC(NULL), mMovieFrameFlags(0),
      mPaletteData(NULL), mLockFlags(0) {
    mMovieFrames[0] = mMovieFrames[1] = NULL;
}
#else
WiiTex::WiiTex() : mImageData(NULL), mFormat() {}
#endif

WiiTex::~WiiTex() { DeleteSurface(); }

void WiiTex::PresyncBitmap() { DeleteSurface(); }

#ifdef VERSION_SZBE69_B8
void WiiTex::DeleteSurface() {
    if (mMovieFrameFlags & 2) {
        mMovieFrameFlags &= ~2;
        WiiRnd::SyncFree(mImageData);
        std::set<WiiTex *>::iterator it = gRenderTextureSet.find(this);
        if (it != gRenderTextureSet.end())
            gRenderTextureSet.erase(it);
    }
    if (mMovieFrameFlags & 4) {
        mMovieFrameFlags &= ~4;
        WiiRnd::SyncFree(mPaletteData);
    }
    if (mMovieFrameFlags & 8) {
        mMovieFrameFlags &= ~8;
        WiiRnd::SyncFree(unk_0xAC);
    }
    if (mMovieFrameFlags & 0x10) {
        mMovieFrameFlags &= ~0x10;
        WiiRnd::SyncFree(mMovieFrames[0]);
        WiiRnd::SyncFree(mMovieFrames[1]);
    }
    mImageData = NULL;
    mPaletteData = NULL;
    unk_0xAC = NULL;
    mMovieFrames[0] = mMovieFrames[1] = NULL;
}
#else
void WiiTex::DeleteSurface() {}
#endif

u32 OrderFromFormat(unsigned int ui) {
    switch (ui) {
    case 4:
        return 0;
    case 14:
        return 72;
    case 6:
        return 64;
    case 1:
        return 192;
    default:
        MILO_FAIL("Currently unsupported format %d for OrderFromFormat.\n", ui);
        return 0;
    }
}

#ifdef VERSION_SZBE69_B8
void WiiTex::Select(_GXTexMapID map) {
    if (!mImageData) {
        static_cast<WiiTex *>(TheRnd->GetNullTexture())->Select(map);
    } else {
        if (mType & kBackBuffer) {
            GXSetTexCopySrc(
                0, 0, TheWiiRnd.mRenderMode.fbWidth, TheWiiRnd.mRenderMode.efbHeight
            );
            GXSetTexCopyDst(mWidth, mHeight, mFormat, true);
            GXCopyTex(mImageData, false);
            GXPixModeSync();
        }
        if (mPaletteData) {
            GXInitTexObjTlut(&mTexObj, map);
            GXLoadTlut(reinterpret_cast<GXTlutObj *>(unk_0xBC), map);
        }
        GXLoadTexObj(&mTexObj, map);
    }
}

void WiiTex::SelectAlpha(_GXTexMapID map) {
    if (mMovieFrameFlags & 1)
        GXLoadTexObj(&mAlphaTexObj, map);
}

void WiiTex::CopyContent(bool clear) {
    CopyFromFB((mMovieFrameFlags & 0x40) ? 0x260 : 0, 0, mWidth, mHeight, clear, false);
}

void WiiTex::MakeDrawTarget() {}

void WiiTex::FinishDrawTarget() {
    if (mType & kRendered)
        CopyContent(true);
}
#endif

#ifdef VERSION_SZBE69_B8
void WiiTex::MovieSwapFrames() {
    if (mMovieFrameFlags & 2) {
        MILO_FAIL("mImageData 0x%08x being leaked!\n", (int)mImageData);
    }
    mImageData = mMovieFrames[(mMovieFrameFlags >> 8) & 1];
    MILO_ASSERT(!((int)mImageData & 31), 604);
    mMovieFrameFlags ^= 0x100;
    DCStoreRange(mImageData, GXGetTexBufferSize(mWidth, mHeight, mFormat, true, 2));
    GXInitTexObjData(&mTexObj, mImageData);
}
#else
void WiiTex::MovieSwapFrames() {}
#endif

#ifdef VERSION_SZBE69_B8
void WiiTex::LockBitmap(RndBitmap &bitmap, int flags) {
    if (!mImageData) {
        RndTex::LockBitmap(bitmap, flags);
    } else {
        bool read = (flags & 1) != 0;
        bool write = (flags & 4) != 0;
        if (read || write) {
            if ((mType & kFrontBuffer) > 0 && read)
                CreateScreenShot();
            bitmap.Create(
                mWidth,
                mHeight,
                0,
                mBpp,
                OrderFromFormat(mFormat),
                mPaletteData,
                mImageData,
                NULL
            );
            if (mMovieFrameFlags & 1) {
                MILO_FAIL("Locking of Wii textures with alpha not currently supported!\n"
                );
            }
            mLockFlags = flags;
        }
    }
}

void WiiTex::UnlockBitmap() {
    if (mImageData) {
        if (mLockFlags & 4) {
            DCStoreRange(
                mImageData, GXGetTexBufferSize(mWidth, mHeight, mFormat, false, 2)
            );
        }
        mLockFlags = 0;
    }
}

void WiiTex::Resize(int width, int height, unsigned long format, bool disableFiltering) {
    mWidth = width;
    mHeight = height;
    mFormat = (GXTexFmt)format;
    GXInitTexObj(
        &mTexObj, mImageData, mWidth, mHeight, mFormat, GX_CLAMP, GX_CLAMP, mNumMips != 0
    );
    DisableFiltering(disableFiltering);
}

void WiiTex::Compress(bool) {
    if ((u32)mFormat == GX_TF_CMPR) {
        MILO_WARN("[ERROR] Trying to compress a compressed texture!\n");
        return;
    }
    int width = mWidth;
    int height = mHeight;
    int mips = mNumMips;
    bool noMips = !mips;
    MILO_ASSERT(!mNumMips, 801);
    int rowBytes = ((width * 4) >> 3) * 4;
    int size = GXGetTexBufferSize(
        width, height, GX_TF_CMPR, (u8)!noMips, (u8)(mips ? mips + 1 : 0)
    );
    u8 *compressed = (u8 *)_MemAlloc(size, 32);
    Dxt1Compress::CompressImage(
        (const u8 *)mImageData, width, height, compressed, (mWidth * mBpp) >> 3, mBpp, 0
    );
    u8 *p = compressed;
    u8 *end = compressed + size;
    while (p < end) {
        EndianSwapEq(*((u16 *&)p)++);
        EndianSwapEq(*((u16 *&)p)++);
        uint indices = *(uint *)p;
        // Reverse the sixteen two-bit selectors, then convert the word byte order.
        *(uint *)p = ((indices & 0x00000003) << 30) | ((indices & 0x0000000C) << 26)
            | ((indices & 0x00000030) << 22) | ((indices & 0x000000C0) << 18)
            | ((indices & 0x00000300) << 14) | ((indices & 0x00000C00) << 10)
            | ((indices & 0x00003000) << 6) | ((indices & 0x0000C000) << 2)
            | ((indices & 0x00030000) >> 2) | ((indices & 0x000C0000) >> 6)
            | ((indices & 0x00300000) >> 10) | ((indices & 0x00C00000) >> 14)
            | ((indices & 0x03000000) >> 18) | ((indices & 0x0C000000) >> 22)
            | ((indices & 0x30000000) >> 26) | ((indices & 0xC0000000) >> 30);
        EndianSwapEq(*(uint *)p);
        p += 4;
    }
    int blocksAcross = rowBytes >> 2;
    int blocksHigh = height >> 3;
    if (blocksHigh && blocksAcross && size > 32 && width > 4 && height > 4) {
        MemDoTempAllocations temp(true, false);
        u8 *copy;
        u8 *secondRow;
        u8 *out;
        u8 *copyEnd;
        u8 *rowEnd;
        u8 *firstRow;
        copy = new u8[size];
        memcpy(copy, compressed, size);
        out = compressed;
        firstRow = copy;
        copyEnd = copy + size;
        rowEnd = firstRow + rowBytes;
        while (firstRow < copyEnd) {
            secondRow = firstRow + rowBytes;
            while (firstRow < rowEnd) {
                memcpy(out, firstRow, 16);
                memcpy(out + 16, secondRow, 16);
                secondRow += 16;
                firstRow += 16;
                out += 32;
            }
            firstRow += rowBytes;
            rowEnd = firstRow + rowBytes;
        }
        delete[] copy;
    }
    DeleteSurface();
    mFormat = GX_TF_CMPR;
    mType = kRegular;
    mBpp = 4;
    if (mMovieFrameFlags & 2)
        WiiRnd::SyncFree(mImageData);
    mImageData = compressed;
    mMovieFrameFlags = (mMovieFrameFlags & ~2) | 2;
    gRenderTextureSet.insert(this);
    GXInitTexObj(
        &mTexObj, mImageData, mWidth, mHeight, mFormat, GX_CLAMP, GX_CLAMP, mNumMips != 0
    );
    SetLOD();
}

void WiiTex::SetTexWrapMode(RndMat::TexWrap s, RndMat::TexWrap t) {
    if ((mType & kBackBuffer) || !mIsPowerOf2) {
        GXInitTexObjWrapMode(&mTexObj, GX_CLAMP, GX_CLAMP);
        GXInitTexObjWrapMode(&mAlphaTexObj, GX_CLAMP, GX_CLAMP);
    } else {
        static GXTexWrapMode op[] = {
            GX_CLAMP, GX_REPEAT, GX_CLAMP, GX_MIRROR, GX_MIRROR
        };
        GXInitTexObjWrapMode(&mTexObj, op[s], op[t]);
        GXInitTexObjWrapMode(&mAlphaTexObj, op[s], op[t]);
    }
}

void WiiTex::SetLOD() {
    if (mMovieFrameFlags & 0x20) {
        GXInitTexObjLOD(
            &mTexObj, (GXTexFilter)0, (GXTexFilter)0, 0, 0, 0, false, false, (GXAnisotropy)0
        );
        if (mMovieFrameFlags & 1)
            GXInitTexObjLOD(
                &mAlphaTexObj,
                (GXTexFilter)0,
                (GXTexFilter)0,
                0,
                0,
                0,
                false,
                false,
                (GXAnisotropy)0
            );
    } else if (mNumMips) {
        MILO_ASSERT(0, 919);
        GXInitTexObjLOD(
            &mTexObj,
            (GXTexFilter)3,
            (GXTexFilter)1,
            0,
            mNumMips,
            0,
            false,
            false,
            (GXAnisotropy)0
        );
        if (mMovieFrameFlags & 1)
            GXInitTexObjLOD(
                &mAlphaTexObj,
                (GXTexFilter)3,
                (GXTexFilter)1,
                0,
                mNumMips,
                0,
                false,
                false,
                (GXAnisotropy)0
            );
    } else {
        GXInitTexObjLOD(
            &mTexObj, (GXTexFilter)1, (GXTexFilter)1, 0, 0, 0, false, false, (GXAnisotropy)0
        );
        if (mMovieFrameFlags & 1)
            GXInitTexObjLOD(
                &mAlphaTexObj,
                (GXTexFilter)1,
                (GXTexFilter)1,
                0,
                0,
                0,
                false,
                false,
                (GXAnisotropy)0
            );
    }
}

void WiiTex::DisableFiltering(bool disabled) {
    mMovieFrameFlags = (mMovieFrameFlags & ~0x20) | (disabled ? 0x20 : 0);
    if (mImageData)
        SetLOD();
}

void *WiiTex::GetMovieLoadingFramePtr() {
    return mMovieFrames[(mMovieFrameFlags >> 8) & 1];
}

int WiiTex::TexelsPitch() const { return mBpp * mWidth; }
#endif

void WiiTex::CopyFromFB(
    int src_x, int src_y, int src_w, int src_h, bool copy_bool, bool is_mip
) {
    MILO_ASSERT(mImageData, 711);
    MILO_ASSERT(mType & kRendered, 712);
    if (copy_bool)
        GXSetZMode(TRUE, GX_ALWAYS, TRUE);
    GXSetAlphaUpdate(TRUE);
#ifdef VERSION_SZBE69_B8
    const Hmx::Color &color = TheRnd->mClearColor;
    uint clearColor = MakeU32Color(color);
    GXSetCopyClear(*(GXColor *)&clearColor, 0x00FFFFFF);
#else
    // TODO add PSVEC copy
    GXSetCopyClear(*(GXColor *)&TheRnd->mClearColor, 0x00FFFFFF);
#endif
    GXSetTexCopySrc(src_x, src_y, src_w, src_h);
    GXSetTexCopyDst(mWidth, mHeight, mFormat, is_mip);
    GXGetTexBufferSize(mWidth, mHeight, mFormat, 0, 0);

    GXSetCopyClamp(GX_CLAMP_ALL);
    GXCopyTex(mImageData, u8(copy_bool));
    if (bComposingOutfitTexture || !TheRnd->mInGame)
        RndGxDrawDone();
}

bool ConvertAndStoreYUV2BMP(void *, int, int, void *);

#ifdef VERSION_SZBE69_B8
void WiiTex::CreateScreenShot() {
    DeleteSurface();
    mWidth = TheWiiRnd.mRenderMode.fbWidth;
    mHeight = TheWiiRnd.mRenderMode.xfbHeight;
    mBpp = 24;
    mImageData = _MemAlloc((mBpp >> 3) * (mWidth * mHeight), 32);
    mMovieFrameFlags = (mMovieFrameFlags & ~2) | 2;
    MILO_ASSERT(!((int)mImageData & 31), 982);
    if (!ConvertAndStoreYUV2BMP(WiiRnd::GetCurrXFB(), mWidth, mHeight, mImageData)) {
        MILO_WARN("[WiiTex::CreateScreenShot] Failed to covert XFB to BMP!\n");
        DeleteSurface();
    }
}
#else
void WiiTex::CreateScreenShot() {
    DeleteSurface();
    if (!ConvertAndStoreYUV2BMP(WiiRnd::GetCurrXFB(), mWidth, mHeight, mImageData)) {
        MILO_WARN("[WiiTex::CreateScreenShot] Failed to covert XFB to BMP!\n"); // BUG:
                                                                                // covert
        DeleteSurface();
    }
}

#endif

#ifdef VERSION_SZBE69_B8
struct YUV422 {
    u8 y, uv;
};
struct YUV444 {
    u8 y, u, v;
};
struct RGB {
    u8 r, g, b;
};
void YUV422To444(YUV422 *, YUV444 *, int, int);
void YUV444ToRGB(YUV444 *, RGB *, int, int);
void RGBToBMP(RGB *, void *, unsigned long, unsigned long);

bool ConvertAndStoreYUV2BMP(void *source, int width, int height, void *destination) {
    int count = width * height;
    YUV444 *yuv = (YUV444 *)calloc(count, sizeof(YUV444));
    if (!yuv)
        return false;
    YUV422To444((YUV422 *)source, yuv, width, height);
    RGB *rgb = (RGB *)calloc(count, sizeof(RGB));
    // Preserve the original second-allocation failure behavior for matching.
    if (!rgb)
        return false;
    YUV444ToRGB(yuv, rgb, width, height);
    free(yuv);
    RGBToBMP(rgb, destination, width, height);
    free(rgb);
    return true;
}

void YUV422To444(YUV422 *source, YUV444 *destination, int width, int height) {
    YUV422 *src;
    YUV444 *dst;
    int x, row;
    for (row = 0; row < height; row++) {
        src = source;
        dst = destination;
        for (x = 0; x < width; x += 2) {
            dst[0].y = src[0].y;
            dst[0].u = src[0].uv;
            dst[0].v = src[1].uv;
            dst[1].y = src[1].y;
            if (x != width - 2) {
                dst[1].u = (src[0].uv + src[2].uv) / 2;
                dst[1].v = (src[1].uv + src[3].uv) / 2;
            } else {
                dst[1].u = src[0].uv;
                dst[1].v = src[1].uv;
            }
            src += 2;
            dst += 2;
        }
        source += width;
        destination += width;
    }
}

void YUV444ToRGB(YUV444 *source, RGB *destination, int width, int height) {
    YUV444 *src;
    RGB *dst;
    int row;
    for (row = 0; row < height; row++) {
        src = source;
        dst = destination;
        for (int x = 0; x < width; x++) {
            int y = src->y - 16;
            int u = src->u - 128;
            int v = src->v - 128;
            int r = (1164 * y + 1596 * v + 500) / 1000;
            int g = (1164 * y - 813 * v - 391 * u + 500) / 1000;
            int b = (1164 * y + 2018 * u + 500) / 1000;
            dst->r = r > 255 ? 255 : (r < 0 ? 0 : r);
            dst->g = g > 255 ? 255 : (g < 0 ? 0 : g);
            dst->b = b > 255 ? 255 : (b < 0 ? 0 : b);
            src++;
            dst++;
        }
        source += width;
        destination += width;
    }
}

void RGBToBMP(RGB *source, void *destination, unsigned long width, unsigned long height) {
    // Original routine stores packed BGR pixels; it does not create a BMP file header.
    u8 *dst = (u8 *)destination;
    int x, row;
    for (row = 0; row < (int)height; row++) {
        for (x = 0; x < (int)width; x++) {
            dst[0] = source[row * width + x].b;
            dst[1] = source[row * width + x].g;
            dst[2] = source[row * width + x].r;
            dst += 3;
        }
    }
}
#else
bool ConvertAndStoreYUV2BMP(void *, int, int, void *) {}
#endif
