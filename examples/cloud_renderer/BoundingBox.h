#ifndef BOUNDINGBOX_H
#define BOUNDINGBOX_H

#include "hammock/hammock.h"

class BoundingBox {
public:


    BoundingBox(const HmckVec3& min, const HmckVec3& max) {
        computeFromCorners(min, max);
    }

    BoundingBox(const HmckVec3& center, float radius) {
        HmckVec3 min = center - HmckVec3{radius,radius,radius};
        HmckVec3 max = center + HmckVec3{radius,radius,radius};
        computeFromCorners(min, max);
    }

    const std::vector<Hammock::Vertex>& getVertices() const { return vertices; }
    const std::vector<uint32_t>& getIndices() const { return indices; }

private:
    std::vector<Hammock::Vertex> vertices;
    std::vector<uint32_t> indices;

    void computeFromCorners(const HmckVec3& min, const HmckVec3& max) {
        vertices = {
            {{min.X, min.Y, min.Z}},
            {{max.X, min.Y, min.Z}},
            {{max.X, max.Y, min.Z}},
            {{min.X, max.Y, min.Z}},
            {{min.X, min.Y, max.Z}},
            {{max.X, min.Y, max.Z}},
            {{max.X, max.Y, max.Z}},
            {{min.X, max.Y, max.Z}}
        };

        indices = {
            0, 3, 2, 2, 1, 0, // Bottom face
            4, 5, 6, 6, 7, 4, // Top face
            0, 4, 7, 7, 3, 0, // Left face
            1, 2, 6, 6, 5, 1, // Right face
            3, 7, 6, 6, 2, 3, // Front face
            0, 1, 5, 5, 4, 0  // Back face
        };
    }
};

#endif