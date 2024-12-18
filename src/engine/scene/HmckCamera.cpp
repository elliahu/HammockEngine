#include "HmckCamera.h"

#include <cassert>
#include <cmath>

HmckMat4 Hmck::Projection::perspective(float fovy, float aspect, float zNear, float zFar, bool flipY) {
    assert(HmckABS(aspect - std::numeric_limits<float>::epsilon()) > 0.0f);
    HmckMat4 perspective =  HmckPerspective_RH_ZO(fovy, aspect, zNear, zFar);
    perspective[0][0] *= flipY ? -1.0f : 1.0f;
    perspective[1][1] *= flipY ? -1.0f : 1.0f;
    return perspective;
}

HmckMat4 Hmck::Projection::view(HmckVec3 eye, HmckVec3 target, HmckVec3 up) {
   return HmckLookAt_RH(eye, target, up);
}

HmckMat4 Hmck::Projection::inverseView(HmckVec3 eye, HmckVec3 target, HmckVec3 up) {
    return HmckInvLookAt(HmckLookAt_RH(eye, target, up));
}
