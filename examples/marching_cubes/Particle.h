#pragma once
#include "half.hpp"
#include <iostream>
#include <fstream>
#include <vector>

struct Particle
{
   half_float::half position_x; // particle position (m)
   half_float::half position_y;
   half_float::half position_z;

   half_float::half velocity_x; // particle velocity (m/s)
   half_float::half velocity_y;
   half_float::half velocity_z;

   half_float::half rho; // density (kg/m3)
   half_float::half pressure;
   half_float::half radius; // particle radius (m)
};

// Function to load binary file into vector of particles
inline bool loadParticles(const std::string& filename, std::vector<Particle>& particles) {
   // Open the binary file
   std::ifstream file(filename, std::ios::binary);
   if (!file) {
      std::cerr << "Error opening file: " << filename << std::endl;
      return false;
   }

   // Seek to the end of the file to get the total size
   file.seekg(0, std::ios::end);
   std::streampos fileSize = file.tellg();
   file.seekg(0, std::ios::beg);

   // Calculate the number of particles based on file size
   std::size_t numParticles = fileSize / sizeof(Particle);

   // Resize the vector to hold all particles
   particles.resize(numParticles);

   // Read the data into the vector
   file.read(reinterpret_cast<char*>(particles.data()), fileSize);

   // Close the file
   file.close();

   return true;
}

// Function to create a scalar field from particles
inline std::vector<std::vector<std::vector<float>>> createScalarField(
    const std::vector<Particle>& particles,
    const float gridSize,   // Grid resolution (distance between grid points)
    const float fieldSize   // Size of the field (bounding box dimensions)
) {
    // Calculate grid dimensions based on the field size and grid resolution
    int gridDim = static_cast<int>(fieldSize / gridSize);

    // Initialize a scalar field with zero values
    std::vector<std::vector<std::vector<float>>> scalarField(
        gridDim, std::vector<std::vector<float>>(
            gridDim, std::vector<float>(gridDim, 0.0f)
        )
    );

    // Populate the scalar field with values based on particle positions
    for (const Particle& particle : particles) {
        // Convert half_float to float for the scalar field calculation
        float x = static_cast<float>(particle.position_x);
        float y = static_cast<float>(particle.position_y);
        float z = static_cast<float>(particle.position_z);
        float rho = static_cast<float>(particle.rho);

        // Determine the grid index for the particle
        int xIdx = static_cast<int>(x / gridSize);
        int yIdx = static_cast<int>(y / gridSize);
        int zIdx = static_cast<int>(z / gridSize);

        // Ensure the indices are within bounds
        if (xIdx >= 0 && xIdx < gridDim &&
            yIdx >= 0 && yIdx < gridDim &&
            zIdx >= 0 && zIdx < gridDim) {
            // Assign the particle's density to the corresponding grid point
            scalarField[xIdx][yIdx][zIdx] += rho;
            }
    }

    return scalarField;
}
