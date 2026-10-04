#pragma once

#include "obj/ObjMacros.h"
#include "revolution/gx/GXTypes.h"
#ifdef VERSION_SZBE69_B8
#include "revolution/gx/GXTexture.h"
#include "rndobj/Mat.h"
#endif
#include "rndobj/Tex.h"
#include "utl/PoolAlloc.h"
#include <set>

/**
 * @brief Platform implementation of RndTex.
 */
class WiiTex : public RndTex {
public:
    WiiTex();
    virtual ~WiiTex();
    OBJ_CLASSNAME(WiiTex)
    OBJ_SET_TYPE(WiiTex)
    NEW_OBJ(WiiTex)
#ifdef VERSION_SZBE69_B8
    virtual void CopyContent(bool);
    virtual void MakeDrawTarget();
    virtual void FinishDrawTarget();
    void SelectAlpha(_GXTexMapID);
#else
    virtual void unk(bool);
#endif

    void PresyncBitmap();
    void DeleteSurface();
    void MovieSwapFrames();
    void CopyFromFB(int, int, int, int, bool, bool);
    void CreateScreenShot();
    void DisableFiltering(bool);
    void Select(_GXTexMapID);
    void *ImgData() { return mImageData; }
    void *GetMovieLoadingFramePtr();
#ifdef VERSION_SZBE69_B8
    int TexelsPitch() const;
    void SetLOD();
    virtual void Compress(bool);
    void SetTexWrapMode(RndMat::TexWrap, RndMat::TexWrap);
    virtual void LockBitmap(RndBitmap &, int);
    virtual void UnlockBitmap();
    void Resize(int, int, unsigned long, bool);
#endif

#ifdef VERSION_SZBE69_B8
    GXTexObj mTexObj; // 0x64
#else
    u8 pad[32];
#endif
    void *mImageData; // 0x84
    GXTexFmt mFormat; // 0x88
#ifdef VERSION_SZBE69_B8
    GXTexObj mAlphaTexObj; // 0x8C
    void *unk_0xAC;
    u32 mMovieFrameFlags; // 0xB0
    void *mMovieFrames[2]; // 0xB4
    u8 unk_0xBC[0xC];
    void *mPaletteData; // 0xC8
    u32 mLockFlags; // 0xCC
#endif

    static bool bComposingOutfitTexture;

    NEW_POOL_OVERLOAD(WiiTex)
    DELETE_POOL_OVERLOAD(WiiTex)
    REGISTER_OBJ_FACTORY_FUNC(WiiTex)
};

extern std::set<WiiTex *> gRenderTextureSet;