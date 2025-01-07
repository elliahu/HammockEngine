#pragma once
#include "half.hpp"
#include <iostream>
#include <fstream>
#include <vector>
#include <iomanip>

struct Particle
{
   half_float::half position_x;
   half_float::half position_y;
   half_float::half position_z;
   half_float::half velocity_x;
   half_float::half velocity_y;
   half_float::half velocity_z;
   half_float::half rho;
   half_float::half pressure;
   half_float::half radius;
};

// Debug function to print raw bytes of a particle
inline void printParticleBytes(const Particle& p) {
    const unsigned char* bytes = reinterpret_cast<const unsigned char*>(&p);
    std::cout << "Raw bytes: ";
    for (size_t i = 0; i < sizeof(Particle); ++i) {
        std::cout << std::hex << std::setw(2) << std::setfill('0')
                  << static_cast<int>(bytes[i]) << " ";
    }
    std::cout << std::dec << std::endl;
}

inline bool loadParticles(const std::string& filename, std::vector<Particle>& particles) {
    std::ifstream file(filename, std::ios::binary);
    if (!file) {
        std::cerr << "Error opening file: " << filename << std::endl;
        return false;
    }

    file.seekg(0, std::ios::end);
    std::streampos fileSize = file.tellg();
    file.seekg(0, std::ios::beg);

    // Print file size and expected number of particles
    std::cout << "File size: " << fileSize << " bytes\n";
    std::cout << "Particle size: " << sizeof(Particle) << " bytes\n";
    std::cout << "Expected number of particles: " << fileSize / sizeof(Particle) << "\n";

    std::size_t numParticles = fileSize / sizeof(Particle);
    particles.resize(numParticles);

    file.read(reinterpret_cast<char*>(particles.data()), fileSize);

    // Debug output for first few particles
    std::cout << "\nFirst 3 particles:\n";
    for (size_t i = 0; i < std::min(size_t(3), particles.size()); ++i) {
        std::cout << "\nParticle " << i << ":\n";
        std::cout << "Position: ("
                  << static_cast<float>(particles[i].position_x) << ", "
                  << static_cast<float>(particles[i].position_y) << ", "
                  << static_cast<float>(particles[i].position_z) << ")\n";
        std::cout << "Velocity: ("
                  << static_cast<float>(particles[i].velocity_x) << ", "
                  << static_cast<float>(particles[i].velocity_y) << ", "
                  << static_cast<float>(particles[i].velocity_z) << ")\n";
        std::cout << "Density: " << static_cast<float>(particles[i].rho) << "\n";
        std::cout << "Pressure: " << static_cast<float>(particles[i].pressure) << "\n";
        std::cout << "Radius: " << static_cast<float>(particles[i].radius) << "\n";

        // Print raw bytes for debugging
        printParticleBytes(particles[i]);
    }

    file.close();
    return true;
}

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

    // Track min/max values for debugging
    float minDensity = std::numeric_limits<float>::max();
    float maxDensity = std::numeric_limits<float>::lowest();
    int particlesAdded = 0;
    int particlesSkipped = 0;

    // Populate the scalar field with values based on particle positions
    for (const Particle& particle : particles) {
        // Convert half_float to float for the scalar field calculation
        float x = static_cast<float>(particle.position_x);
        float y = static_cast<float>(particle.position_y);
        float z = static_cast<float>(particle.position_z);
        float rho = static_cast<float>(particle.rho);

        // Map from [-fieldSize/2, fieldSize/2] to [0, gridDim]
        float normalizedX = (x + fieldSize/2.0f) / fieldSize * gridDim;
        float normalizedY = (y + fieldSize/2.0f) / fieldSize * gridDim;
        float normalizedZ = (z + fieldSize/2.0f) / fieldSize * gridDim;

        // Convert to indices
        int xIdx = static_cast<int>(normalizedX);
        int yIdx = static_cast<int>(normalizedY);
        int zIdx = static_cast<int>(normalizedZ);

        // Ensure the indices are within bounds
        if (xIdx >= 0 && xIdx < gridDim &&
            yIdx >= 0 && yIdx < gridDim &&
            zIdx >= 0 && zIdx < gridDim) {
            // Assign the particle's density to the corresponding grid point
            scalarField[xIdx][yIdx][zIdx] += rho;

            // Update min/max values
            minDensity = std::min(minDensity, scalarField[xIdx][yIdx][zIdx]);
            maxDensity = std::max(maxDensity, scalarField[xIdx][yIdx][zIdx]);
            particlesAdded++;
        } else {
            particlesSkipped++;
        }
    }

    // Print debug information
    std::cout << "Scalar field creation stats:\n";
    std::cout << "Grid dimensions: " << gridDim << "x" << gridDim << "x" << gridDim << "\n";
    std::cout << "Particles added: " << particlesAdded << "\n";
    std::cout << "Particles skipped: " << particlesSkipped << "\n";
    std::cout << "Min density in grid: " << minDensity << "\n";
    std::cout << "Max density in grid: " << maxDensity << "\n";

    return scalarField;
}