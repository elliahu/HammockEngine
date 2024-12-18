#pragma once
#include <cstdint>
#include <vector>
#include <HandmadeMath.h>

#include "core/HmckDeviceStorage.h"
#include "scene/HmckVertex.h"

#define CHILD_PREALLOC_COUNT 4

namespace Hmck {
    struct State {

        static constexpr size_t MAX_MESHES = 256;
        typedef int32_t MeshHandle;
        typedef int32_t Index;

        enum VisibilityFlags : int32_t {
            NONE = 0,
            VISIBLE = 1 << 0,
            OPAQUE = 1 << 1,
            TRANSPARENT = 1 << 1,
            CASTS_SHADOW = 1 << 3,
            RECIEVES_SHADOW = 1 << 4,
        };

        struct RenderMesh {
            HmckMat4 transform;
            int32_t visibilityFlags;
            HmckVec3 baseColorFactor;
            HmckVec3 metallicRoughnessAlphaCutOffFactor;
            Index baseColorTextureIndex;
            Index normalTextureIndex;
            Index metallicRoughnessTextureIndex;
            Index occlusionTextureIndex;
            uint32_t firstIndex;
            uint32_t indexCount;
        };

        std::vector<RenderMesh> renderMeshes;
        std::vector<Vertex> vertices;
        std::vector<uint32_t> indices;
        std::vector<Texture2DHandle> textures;

    };
}
