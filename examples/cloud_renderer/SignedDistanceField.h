#ifndef SIGNED_DISTANCE_FIELD_H
#define SIGNED_DISTANCE_FIELD_H

#include <cstdint>
#include <fstream>
#include <string>
#include <vector>

class SignedDistanceField {
public:
    SignedDistanceField &loadFromFile(const std::string &filename, int32_t &gridSize) {
        // Open the binary file
        std::ifstream file(filename, std::ios::binary);

        assert(file.is_open());

        // Read the grid size (first 4 bytes are the unsigned int grid size)
        file.read(reinterpret_cast<char*>(&gridSize), sizeof(int));

        // Calculate the total number of elements (grid_size^3 for a cubic grid)
        size_t total_elements = gridSize * gridSize * gridSize;

        // Allocate memory for the SDF grid
        field = new float[total_elements];

        // Read the SDF values into the array
        file.read(reinterpret_cast<char*>(field), total_elements * sizeof(float));

        assert(!file.fail());

        file.close();

        return *this;
    }

    [[nodiscard]] float *data() const {
        return field;
    }


    void free() const {
        delete[] field;
    }

private:
    float *field;
};

#endif
