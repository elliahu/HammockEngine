#ifndef NOISE3D_H
#define NOISE3D_H

#include <iostream>
#include <memory>
#include <algorithm>
#include <array>
#include "FastNoiseLite.h"  // Include FastNoiseLite

// Abstract Base Class for 3D Noise
class Noise3D {
public:
    virtual ~Noise3D() = default;
    virtual void setSeed(int seed) = 0;
    virtual void setFrequency(float frequency) = 0;
    virtual float getNoise(float x, float y, float z) const = 0;
};

// Cellular Noise (Worley Noise) Implementation
class CellularNoise3D : public Noise3D {
private:
    FastNoiseLite noise;
    float frequency;

public:
    CellularNoise3D(int seed = 0, float freq = 0.01f)
        : frequency(freq) {
        noise.SetSeed(seed);
        noise.SetNoiseType(FastNoiseLite::NoiseType_Cellular);
        noise.SetFrequency(frequency);
    }

    void setSeed(int seed) override { noise.SetSeed(seed); }
    void setFrequency(float freq) override { frequency = freq; noise.SetFrequency(frequency); }

    float getNoise(float x, float y, float z) const override {
        return noise.GetNoise(x, y, z);
    }
};

// Multi-Channel Noise System with Separate Settings per Channel
class MultiChannelNoise3D {
private:
    struct ChannelSettings {
        int seed;
        float frequency;
    };

    std::vector<std::unique_ptr<CellularNoise3D>> noiseChannels;  // Store noise for each channel
    std::vector<ChannelSettings> channelSettings;  // Settings for each channel
    int numChannels;
    float minValue, maxValue;  // For scaling

public:
    MultiChannelNoise3D(
        const std::vector<ChannelSettings>& settings,  // Separate settings per channel
        float minValue = 0.0f,
        float maxValue = 1.0f)
        : numChannels(settings.size()), minValue(minValue), maxValue(maxValue) {

        // Create the noise channels with individual settings
        for (const auto& setting : settings) {
            noiseChannels.push_back(std::make_unique<CellularNoise3D>(setting.seed, setting.frequency));
        }
    }

    // Function to scale the values to a custom interval
    float scaleValue(float value) const {
        // Normalize value from [-1, 1] to [0, 1]
        float normalizedValue = (value + 1.0f) * 0.5f;

        // Then scale to [minValue, maxValue]
        return minValue + normalizedValue * (maxValue - minValue);
    }

    // Generate the noise texture buffer
    float* getTextureBuffer(int width, int height, int depth) const {
        float* buffer = new float[width * height * depth * numChannels];  // Allocate for variable channels

        int idx = 0;
        for (int z = 0; z < depth; ++z) {
            for (int y = 0; y < height; ++y) {
                for (int x = 0; x < width; ++x) {
                    // Generate noise values for each channel and scale them
                    for (int c = 0; c < numChannels; ++c) {
                        float noiseValue = noiseChannels[c]->getNoise(x, y, z);
                        float scaledValue = scaleValue(noiseValue); // Scale to the defined range
                        buffer[idx++] = scaledValue;
                    }
                }
            }
        }

        return buffer;  // Caller is responsible for deleting the buffer
    }
};
#endif
