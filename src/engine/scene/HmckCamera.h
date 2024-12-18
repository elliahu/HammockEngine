#pragma once

#include "HandmadeMath.h"

namespace Hmck {

    class Projection {
    public:
        HmckMat4 perspective(float fovy, float aspect, float zNear, float zFar);
        HmckMat4 view(HmckVec3 eye, HmckVec3 target, HmckVec3 up);
        HmckMat4 inverseView(HmckVec3 eye, HmckVec3 target, HmckVec3 up);

    };
}
