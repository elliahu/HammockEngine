#pragma once

#include "HandmadeMath.h"

namespace Hmck {
    class Projection {
    public:
        HmckVec3 upNegY() { return HmckVec3{0.f, -1.f, 0.f}; }
        HmckVec3 upPosY() { return HmckVec3{0.f, 1.f, 0.f}; }

        HmckMat4 perspective(float fovy, float aspect, float zNear, float zFar);

        HmckMat4 view(HmckVec3 eye, HmckVec3 target, HmckVec3 up);

        HmckMat4 inverseView(HmckVec3 eye, HmckVec3 target, HmckVec3 up);
    };
}
