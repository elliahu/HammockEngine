#include "hammock/scene/Camera.h"

#include <cassert>
#include <cmath>

HmckMat4 Hmck::Projection::perspective(float fovy, float aspect, float zNear, float zFar, bool flip ) {
    assert(HmckABS(aspect - std::numeric_limits<float>::epsilon()) > 0.0f);
    HmckMat4 projection = HmckPerspective_RH_ZO(fovy, aspect, zNear, zFar);
    if (flip) {
        projection[1][1] *= -1;
    }
    return projection;
}

HmckMat4 Hmck::Projection::view(HmckVec3 eye, HmckVec3 target, HmckVec3 up) {
   return HmckLookAt_RH(eye, target, up);
}

HmckMat4 Hmck::Projection::inverseView(HmckVec3 eye, HmckVec3 target, HmckVec3 up) {
    return HmckInvLookAt(HmckLookAt_RH(eye, target, up));
}
