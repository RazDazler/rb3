#include "Cam.h"

Transform WiiCam::sViewToWiiViewXfm;
Transform WiiCam::sWiiViewToViewXfm;

WiiCam::WiiCam() {}

void WiiCam::Select() {}

#ifdef VERSION_SZBE69_B8
u32 WiiCam::ProjectZ(float z) {
    float nearPlane = mNearPlane;
    float farPlane = mFarPlane;
    float minZ = mZRange.x;
    float distance = farPlane - nearPlane;
    float maxZ = mZRange.y;
    float range = maxZ - minZ;
    float farScale = farPlane / distance;
    float projected = farScale * nearPlane;
    projected = z * farScale - projected;
    z = projected / z;
    z = z * range + minZ;
    return 16777215.0f * z;
}
#else
u32 WiiCam::ProjectZ(float f1) {
    float f3 = mNearPlane - mFarPlane;
    float f2 = mZRange.y - mZRange.x;
    float f5 = mFarPlane / f3;
    float f3_2 = mNearPlane * mFarPlane;
    f3_2 = f1 * f5 - f3_2;
    f1 = f3_2 / f1;
    f1 = f1 * f2 + mZRange.x;
    return 16777215 * f1;
}

#endif
