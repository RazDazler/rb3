#include "Rot.h"
#include "math/Mtx.h"
#include "os/Debug.h"
#include "math/Utl.h"
#include "utl/BinStream.h"
#include <cmath>

void deadstripped_assert() { MILO_ASSERT(false, 0); }

#ifdef VERSION_SZBE69
#pragma push
#pragma dont_inline off
#pragma inline_depth(1)
#pragma inline_max_size(1)
Transform &TransformNoScale::ToTransform(Transform &tf) const {
    Hmx::Quat quat;
    q.ToQuat(quat);
    MakeRotMatrix(quat, tf.m);
    tf.v = v;
    return tf;
}
#pragma pop

// Keep retail weak helpers emitted by the original transform routines.
DECOMP_FORCEBLOCK(Rot, (const Vector3 *a, const Vector3 *b, BinStream *bs),
    *a == *b;
    *a != *b;
    *bs << *a;
)

void TransformNoScale::Set(const Transform &tf) {
    q.Set(tf.m);
    v = tf.v;
}

void TransformNoScale::Set(const TransformNoScale &tf) {
    q = tf.q;
    v = tf.v;
}

void ShortQuat::Set(const Hmx::Quat &quat) {
    x = std::floor(Clamp(-32767.0f, 32767.0f, 32767.0f * quat.x + 0.5f));
    y = std::floor(Clamp(-32767.0f, 32767.0f, 32767.0f * quat.y + 0.5f));
    z = std::floor(Clamp(-32767.0f, 32767.0f, 32767.0f * quat.z + 0.5f));
    w = std::floor(Clamp(-32767.0f, 32767.0f, 32767.0f * quat.w + 0.5f));
}

void ShortQuat::Set(const Hmx::Matrix3 &mtx) { Set(Hmx::Quat(mtx)); }

Hmx::Quat &TransformNoScale::GetRot(Hmx::Quat &quat) const {
    q.ToQuat(quat);
    return quat;
}

void TransformNoScale::SetRot(const Hmx::Quat &quat) { q.Set(quat); }
float ATan2Thunk(float, float);

void MakeEuler(const Hmx::Matrix3 &mtx, Vector3 &euler) {
    if (std::fabs(mtx.y.z) > 0.99999988f) {
        if (mtx.y.z > 0.0f)
            euler.x = PI / 2.0f;
        else
            euler.x = -PI / 2.0f;
        euler.z = ATan2Thunk(mtx.x.y, mtx.x.x);
        euler.y = 0.0f;
    } else {
        euler.z = ATan2Thunk(-mtx.y.x, mtx.y.y);
        euler.x = asinf(mtx.y.z);
        euler.y = ATan2Thunk(-mtx.x.z, mtx.z.z);
    }
}

void Hmx::Quat::Set(const Vector3 &axis, float angle) {
    float half = 0.5f * angle;
    float sine = Sine(half);
    w = Cosine(half);
    x = axis.x * sine;
    y = axis.y * sine;
    z = axis.z * sine;
}

void Multiply(const Hmx::Quat &a, const Hmx::Quat &b, Hmx::Quat &result) {
    result.Set(
        a.w * b.x + a.x * b.w + a.y * b.z - a.z * b.y,
        a.w * b.y + a.y * b.w + a.z * b.x - a.x * b.z,
        a.w * b.z + a.z * b.w + a.x * b.y - a.y * b.x,
        a.w * b.w - a.x * b.x - a.y * b.y - a.z * b.z
    );
}

inline float operator*(const Hmx::Quat &a, const Hmx::Quat &b) {
    return a.x * b.x + a.y * b.y + a.z * b.z + a.w * b.w;
}

void FastInterp(
    const Hmx::Quat &from, const Hmx::Quat &to, float amount, Hmx::Quat &result
) {
    if (amount == 0.0f)
        result = from;
    else if (amount == 1.0f)
        result = to;
    else {
        if (from * to < 0.0f) {
            {
                float start = from.x;
                result.x = start - amount * (to.x + start);
            }
            {
                float start = from.y;
                result.y = start - amount * (to.y + start);
            }
            {
                float start = from.z;
                result.z = start - amount * (to.z + start);
            }
            {
                float start = from.w;
                result.w = start - amount * (to.w + start);
            }
        } else {
            {
                float start = from.x;
                result.x = amount * (to.x - start) + start;
            }
            {
                float start = from.y;
                result.y = amount * (to.y - start) + start;
            }
            {
                float start = from.z;
                result.z = amount * (to.z - start) + start;
            }
            {
                float start = from.w;
                result.w = amount * (to.w - start) + start;
            }
        }
        Normalize(result, result);
    }
}

Hmx::Quat::Quat(const Vector3 &axis, float angle) { Set(axis, angle); }

#pragma push
#pragma dont_inline off
#pragma inline_depth(1)
#pragma inline_max_size(1)
void Hmx::Quat::Set(const Vector3 &euler) {
    Vector3 half;
    Scale(euler, 0.5f, half);
    float sx = Sine(half.x);
    float cx = Cosine(half.x);
    float sy = Sine(half.y);
    float cy = Cosine(half.y);
    Set(sx * cy, cx * sy, sx * sy, cx * cy);
    float sz = Sine(half.z);
    float cz = Cosine(half.z);
    float qw = w;
    float qz = z;
    float qx = x;
    float qy = y;
    Set(cz * qx - sz * qy, cz * qy + sz * qx, cz * qz + sz * qw, cz * qw - sz * qz);
}
#pragma pop

void IdentityInterp(const Hmx::Quat &from, float amount, Hmx::Quat &result) {
    if (amount == 0.0f) {
        result = from;
        return;
    }
    if (amount == 1.0f) {
        result.Set(0.0f, 0.0f, 0.0f, 1.0f);
        return;
    }
    float remaining = 1.0f - amount;
    result.x = from.x * remaining;
    result.y = from.y * remaining;
    result.z = from.z * remaining;
    if (from.w < 0.0f)
        result.w = from.w * remaining - amount;
    else
        result.w = from.w * remaining + amount;
    Normalize(result, result);
}

void FastInvert(const Hmx::Matrix3 &mtx, Hmx::Matrix3 &inverse) {
    float x = 1.0f / Dot(mtx.x, mtx.x);
    float y = 1.0f / Dot(mtx.y, mtx.y);
    float z = 1.0f / Dot(mtx.z, mtx.z);
    inverse.Set(
        mtx.x.x * x,
        mtx.y.x * y,
        mtx.z.x * z,
        mtx.x.y * x,
        mtx.y.y * y,
        mtx.z.y * z,
        mtx.x.z * x,
        mtx.y.z * y,
        mtx.z.z * z
    );
}

void MakeRotQuatUnitX(const Vector3 &vec, Hmx::Quat &quat) {
    float w = std::sqrt(0.5f * vec.x + 0.5f);
    if (w > 1.0e-7f) {
        float scale = 0.5f / w;
        quat.Set(0.0f, vec.z * scale, -vec.y * scale, w);
    } else {
        quat.Set(0.0f, 0.0f, 1.0f, 0.0f);
    }
}

#pragma push
#pragma dont_inline off
#pragma inline_depth(1)
#pragma inline_max_size(1)
void MakeRotQuat(const Vector3 &from, const Vector3 &to, Hmx::Quat &quat) {
    Vector3 cross;
    Cross(from, to, cross);
    float length = std::sqrt(LengthSquared(from) * LengthSquared(to));
    float w = std::sqrt(0.5f + 0.5f * Dot(from, to) / length);
    if (w > 1.0e-7f) {
        float scale = 0.5f / (length * w);
        quat.x = cross.x * scale;
        quat.y = cross.y * scale;
        quat.z = cross.z * scale;
        quat.w = w;
    } else {
        quat.Set(0.0f, 0.0f, 1.0f, 0.0f);
    }
}

#pragma pop

void MakeVertical(Hmx::Matrix3 &mtx) {
    mtx.z.Set(0.0f, 0.0f, 1.0f);
    mtx.y.z = 0.0f;
    Normalize(mtx.y, mtx.y);
    Cross(mtx.y, mtx.z, mtx.x);
}

#pragma push
#pragma dont_inline off
#pragma inline_depth(1)
#pragma inline_max_size(1)
void MakeScale(const Hmx::Matrix3 &mtx, Vector3 &scale) {
    float z = Length(mtx.z);
    Vector3 cross;
    Cross(mtx.x, mtx.y, cross);
    z = Dot(cross, mtx.z) > 0.0f ? z : -z;
    scale.Set(Length(mtx.x), Length(mtx.y), z);
}

#pragma pop

void MakeEulerScale(const Hmx::Matrix3 &mtx, Vector3 &euler, Vector3 &scale) {
    MakeScale(mtx, scale);
    Hmx::Matrix3 normalized;
    if (scale.x != 0.0f)
        Scale(mtx.x, 1.0f / scale.x, normalized.x);
    if (scale.y != 0.0f)
        Scale(mtx.y, 1.0f / scale.y, normalized.y);
    if (scale.z != 0.0f)
        Scale(mtx.z, 1.0f / scale.z, normalized.z);
    MakeEuler(normalized, euler);
}

#else
void TransformNoScale::SetRot(const Hmx::Matrix3 &m) {
    Hmx::Quat quat;
    quat.Set(m);

    float nu_x = 32767.0f * quat.x + 0.5f;
    q.x = std::floor(nu_x > quat.x ? nu_x : (nu_x < -32767.0f ? nu_x : quat.x));

    float nu_y = 32767.0f * quat.y + 0.5f;
    q.y = std::floor(nu_y > quat.y ? nu_y : (nu_y < -32767.0f ? nu_y : quat.y));

    float nu_z = 32767.0f * quat.z + 0.5f;
    q.z = std::floor(nu_z > quat.z ? nu_z : (nu_z < -32767.0f ? nu_z : quat.z));

    float nu_w = 32767.0f * quat.w + 0.5f;
    q.w = std::floor(nu_w > quat.w ? nu_w : (nu_w < -32767.0f ? nu_w : quat.w));
}
#endif

void TransformNoScale::Reset() {
#ifdef VERSION_SZBE69
    q.x = q.y = q.z = 0;
    q.w = 32767;
    v.x = v.y = v.z = 0.0f;
#else
    q.Reset();
    v.Zero();
#endif
}

BinStream &operator>>(BinStream &bs, TransformNoScale &t) {
    Hmx::Matrix3 m;
#ifdef VERSION_SZBE69
    bs >> m >> t.v;
    t.q.Set(m);
#else
    bs >> m;
    bs >> t.v;
    t.SetRot(m);
#endif
    return bs;
}

float GetXAngle(const Hmx::Matrix3 &m) {
    float z = m.y.z;
    return std::atan2(z, m.y.y);
}

float GetYAngle(const Hmx::Matrix3 &m) {
    float z = -m.x.z;
    return std::atan2(z, m.z.z);
}

float GetZAngle(const Hmx::Matrix3 &m) {
    float x = m.y.x;
    return -std::atan2(x, m.y.y);
}

TextStream &operator<<(TextStream &ts, const Hmx::Quat &v) {
    ts << "(x:" << v.x << " y:" << v.y << " z:" << v.z << " w:" << v.w << ")";
    return ts;
}

TextStream &operator<<(TextStream &ts, const Vector3 &v) {
    ts << "(x:" << v.x << " y:" << v.y << " z:" << v.z << ")";
    return ts;
}

TextStream &operator<<(TextStream &ts, const Vector2 &v) {
    ts << "(x:" << v.x << " y:" << v.y << ")";
    return ts;
}

TextStream &operator<<(TextStream &ts, const Hmx::Matrix3 &m) {
    ts << "\n\t" << m.x << "\n\t" << m.y << "\n\t" << m.z;
    return ts;
}

TextStream &operator<<(TextStream &ts, const Transform &t) {
    ts << t.m << "\n\t" << t.v;
    return ts;
}
