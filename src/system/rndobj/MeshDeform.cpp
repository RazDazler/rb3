#include "rndobj/MeshDeform.h"
#include "decomp.h"

#if defined(VERSION_SZBE69_B8)
// Preserve original literal ordering while incomplete methods remain.
DECOMP_FORCEACTIVE(
    LiteralPoolMeshDeform,
    "MeshDeform.cpp",
    "vert <= End()",
    "from >= 0 && from < num",
    "can't copy vert past end",
    "RndMeshDeform",
    "num < VertArray::kMaxWeights",
    "%s vert %d has negative weight %g on bone, won't export",
    "%s vert %d weights sum to %g, check the skinning",
    "0",
    "%s could not find bone, using zero",
    "md->Mesh() == m",
    "exo_",
    "%s null bone %d\n",
    "%s cannot reskin %s, the vert counts differ mesh:%d me:%d",
    "ObjPtr_p.h",
    "f.Owner()",
    "",
    "num_verts ",
    "\n",
    "mesh_inverse ",
    "skip_inverse ",
    "mesh ",
    "bone",
    ":\n",
    "   ",
    "weights",
    ": ",
    "(",
    " ",
    ") ",
    "%s can't load new %s version %d > %d",
    "%s can't load new %s alt version %d > %d",
    "%s(%d): %s unhandled msg: %s",
    "PropSync_p.h",
    "i == prop->Size() && op <= kPropInsert"
)
#endif
#include "obj/ObjMacros.h"
#include "obj/Object.h"
#include "rndobj/Mesh.h"

INIT_REVS(RndMeshDeform)

RndMeshDeform::VertArray::VertArray(RndMeshDeform *md)
    : mSize(0), mData(0), mParent(md) {}

RndMeshDeform::VertArray::~VertArray() { _MemFree(mData); }

void RndMeshDeform::VertArray::Clear() { SetSize(0); }

int RndMeshDeform::VertArray::NumVerts() {
    u8 *buf = (u8 *)mData;
    void *end = (void *)((int)mData + mSize);
    int i = 0;
    for (; buf < end;) {
        i++;
        buf = (*buf << 1) + buf;
        buf++;
    }
    return i;
}

void RndMeshDeform::VertArray::SetSize(int i) {
    if (mSize != i) {
        mSize = i;
        _MemFree(mData);
        mData = _MemAlloc(mSize, 0);
    }
}

void RndMeshDeform::VertArray::Copy(const RndMeshDeform::VertArray &other) {
    SetSize(other.mSize);
    memcpy(mData, other.mData, mSize);
}

void RndMeshDeform::VertArray::Load(BinStream &bs) {
    int siz;
    bs >> siz;
    SetSize(siz);
    bs.Read(mData, mSize);
}

RndMeshDeform::RndMeshDeform()
    : mMesh(this, 0), mBones(this), mVerts(this), mSkipInverse(0), mDeformed(0) {}

RndMeshDeform::~RndMeshDeform() {}

void RndMeshDeform::SetMesh(RndMesh *mesh) {
    mMesh = mesh;
    mVerts.Clear();
}

BinStream &operator>>(BinStream &bs, RndMeshDeform::BoneDesc &) { return bs; }

SAVE_OBJ(RndMeshDeform, 532)

void RndMeshDeform::Print() {}

BEGIN_LOADS(RndMeshDeform)
    LOAD_REVS(bs)
    ASSERT_REVS(3, 0)
    LOAD_SUPERCLASS(Hmx::Object)
    bs >> mMesh;
    int what = 0;
    if (gRev < 1) {
        bs >> what;
    }
    int bones;
    bs >> bones;
    if (gRev < 1) {
        mVerts.Clear();
        for (int i = 0; i < what; i++) {
        }
    }
    SetNumBones(bones);
    for (int i = 0; i < bones; i++) {
        bs >> mBones[i];
    }
    if (gRev != 0)
        mVerts.Load(bs);
    bs >> mMeshInverse;
    // how NOT to check against the identity matrix
    mSkipInverse =
        (0 == mMeshInverse.v.x && 0 == mMeshInverse.v.y && 0 == mMeshInverse.v.z
         && 1 == mMeshInverse.m.x.x && 0 == mMeshInverse.m.x.y && 0 == mMeshInverse.m.x.z
         && 0 == mMeshInverse.m.y.x && 1 == mMeshInverse.m.y.y && 0 == mMeshInverse.m.y.z
         && 0 == mMeshInverse.m.z.x && 0 == mMeshInverse.m.z.y
         && 1 == mMeshInverse.m.z.z);
END_LOADS

BEGIN_HANDLERS(RndMeshDeform)
    HANDLE_SUPERCLASS(Hmx::Object)
    HANDLE_CHECK(0x2A1)
END_HANDLERS