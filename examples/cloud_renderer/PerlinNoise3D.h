#pragma once
#include <algorithm>
#include <vector>
#include <random>
#include <cmath>
#include <numeric>

class PerlinNoise3D {
public:
    PerlinNoise3D();

    PerlinNoise3D(unsigned int seed);

    double noise(double x, double y, double z);

    double octaveNoise(double x, double y, double z, int octaves, double persistence = 0.5);

    // Generate noise volume and return a float buffer
    float *generateNoiseVolume(
        int width, int height, int depth,
        double scale = 1.0,
        int octaves = 1,
        double persistence = 0.5);

private:
    std::vector<int> p;

    double fade(double t) {
        return t * t * t * (t * (t * 6 - 15) + 10);
    }

    double lerp(double t, double a, double b) {
        return a + t * (b - a);
    }

    double grad(int hash, double x, double y, double z) {
        int h = hash & 15;
        double u = h < 8 ? x : y;
        double v = h < 4 ? y : h == 12 || h == 14 ? x : z;
        return ((h & 1) == 0 ? u : -u) + ((h & 2) == 0 ? v : -v);
    }
};
