#pragma once
#include <cstdint>
#include <HandmadeMath.h>

namespace Hmck {

    struct alignas(16) IntPadding {
        int32_t value;
        int32_t padding[3]; // Explicit padding to 16 bytes
    };

    struct alignas(16) FloatPadding {
        float value;
        float padding[3];
    };

    struct alignas(16) Vec3Padding {
        HmckVec3 value;
        float padding;
    };

    // Follows std140 alignment
    struct alignas(16) GlobalDataBuffer {
        static constexpr size_t MAX_MESHES = 256;

        alignas(16) HmckVec4 baseColorFactors[MAX_MESHES]; // w is padding
        alignas(16) HmckVec4 metallicRoughnessAlphaCutOffFactors[MAX_MESHES]; // w is padding

        IntPadding baseColorTextureIndexes[MAX_MESHES];
        IntPadding normalTextureIndexes[MAX_MESHES];
        IntPadding metallicRoughnessTextureIndexes[MAX_MESHES];
        IntPadding occlusionTextureIndexes[MAX_MESHES];
        IntPadding visibilityFlags[MAX_MESHES];
    };

    // Projection buffer bound every frame
    struct FrameDataBuffer {
        HmckMat4 projectionMat{};
        HmckMat4 viewMat{};
        HmckMat4 inverseViewMat{};
        HmckVec4 exposureGammaWhitePoint{4.5f, 1.0f, 11.0f};
    };

    // Push block pushed for each mesh
    struct PushBlockDataBuffer {
        HmckMat4 modelMat{};
        uint32_t meshIndex = -1;
    };
}
