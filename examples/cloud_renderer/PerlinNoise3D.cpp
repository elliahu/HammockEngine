#include "PerlinNoise3D.h"

PerlinNoise3D::PerlinNoise3D() {
    p.resize(256);
    std::iota(p.begin(), p.end(), 0);
    std::random_device rd;
    std::mt19937 gen(rd());
    std::shuffle(p.begin(), p.end(), gen);
    p.insert(p.end(), p.begin(), p.end());
}

PerlinNoise3D::PerlinNoise3D(unsigned int seed) {
    p.resize(256);
    std::iota(p.begin(), p.end(), 0);
    std::mt19937 gen(seed);
    std::shuffle(p.begin(), p.end(), gen);
    p.insert(p.end(), p.begin(), p.end());
}

double PerlinNoise3D::noise(double x, double y, double z) {
    int X = static_cast<int>(floor(x)) & 255;
    int Y = static_cast<int>(floor(y)) & 255;
    int Z = static_cast<int>(floor(z)) & 255;

    x -= floor(x);
    y -= floor(y);
    z -= floor(z);

    double u = fade(x);
    double v = fade(y);
    double w = fade(z);

    int A  = p[X] + Y;
    int AA = p[A] + Z;
    int AB = p[A + 1] + Z;
    int B  = p[X + 1] + Y;
    int BA = p[B] + Z;
    int BB = p[B + 1] + Z;

    return lerp(w,
                lerp(v,
                     lerp(u, grad(p[AA], x, y, z),
                          grad(p[BA], x-1, y, z)),
                     lerp(u, grad(p[AB], x, y-1, z),
                          grad(p[BB], x-1, y-1, z))),
                lerp(v,
                     lerp(u, grad(p[AA+1], x, y, z-1),
                          grad(p[BA+1], x-1, y, z-1)),
                     lerp(u, grad(p[AB+1], x, y-1, z-1),
                          grad(p[BB+1], x-1, y-1, z-1))));
}

double PerlinNoise3D::octaveNoise(double x, double y, double z, int octaves, double persistence) {
    double total = 0;
    double frequency = 1;
    double amplitude = 1;
    double maxValue = 0;

    for(int i = 0; i < octaves; i++) {
        total += noise(x * frequency, y * frequency, z * frequency) * amplitude;
        maxValue += amplitude;
        amplitude *= persistence;
        frequency *= 2;
    }

    return total / maxValue;
}

float * PerlinNoise3D::generateNoiseVolume(int width, int height, int depth, double scale, int octaves,
                                           double persistence) {

    // Allocate the buffer
    float* buffer = new float[width * height * depth];

    // Fill the buffer with noise values
    for(int x = 0; x < width; x++) {
        for(int y = 0; y < height; y++) {
            for(int z = 0; z < depth; z++) {
                double nx = x * scale;
                double ny = y * scale;
                double nz = z * scale;

                // Calculate 1D index from 3D coordinates
                int index = x + y * width + z * width * height;

                // Store noise value in buffer
                buffer[index] = static_cast<float>(
                    octaveNoise(nx, ny, nz, octaves, persistence)
                );
            }
        }
    }

    return buffer;
}
