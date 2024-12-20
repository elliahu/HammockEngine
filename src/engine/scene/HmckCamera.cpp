#include "HmckCamera.h"

#include <cassert>
#include <cmath>

HmckMat4 Hmck::Projection::perspective(float fovy, float aspect, float zNear, float zFar) {
    assert(HmckABS(aspect - std::numeric_limits<float>::epsilon()) > 0.0f);
    return HmckPerspective_RH_ZO(fovy, aspect, zNear, zFar);
}

HmckMat4 Hmck::Projection::view(HmckVec3 eye, HmckVec3 target, HmckVec3 up) {
   return HmckLookAt_RH(eye, target, up);
}

HmckMat4 Hmck::Projection::inverseView(HmckVec3 eye, HmckVec3 target, HmckVec3 up) {
    return HmckInvLookAt(HmckLookAt_RH(eye, target, up));
}
