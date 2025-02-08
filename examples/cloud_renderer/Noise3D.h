#ifndef NOISE3D_H
#define NOISE3D_H
#include <iostream>
#include <memory>
#include <algorithm>
#include <array>
#include "FastNoiseLite.h"  // Include FastNoiseLite
#include <vector>

// Abstract Base Class for 3D Noise
class Noise3D {
public:
    virtual ~Noise3D() = default;
    virtual void setSeed(int seed) = 0;
    virtual void setFrequency(float frequency) = 0;
    virtual float getNoise(float x, float y, float z) const = 0;
    virtual float getTiledNoise(float x, float y, float z, float period) const = 0;
    virtual float getMirroredNoise(float x, float y, float z, float tileSize) = 0;
};

// Generic Noise Class using FastNoiseLite, allowing different noise types
class GenericNoise3D : public Noise3D {
private:
    FastNoiseLite noise;
    float frequency;
    FastNoiseLite::NoiseType noiseType;

public:
    GenericNoise3D(FastNoiseLite::NoiseType type = FastNoiseLite::NoiseType_OpenSimplex2, int seed = 0, float freq = 0.01f)
        : noiseType(type), frequency(freq) {
        noise.SetSeed(seed);
        noise.SetNoiseType(noiseType);
        noise.SetFrequency(frequency);
    }

    void setSeed(int seed) override { noise.SetSeed(seed); }
    void setFrequency(float freq) override { frequency = freq; noise.SetFrequency(frequency); }
    void setNoiseType(FastNoiseLite::NoiseType type) { noiseType = type; noise.SetNoiseType(type); }

    float getNoise(float x, float y, float z) const override {
        return noise.GetNoise(x, y, z);
    }

    float getTiledNoise(float x, float y, float z, float period) const override{
        float nx = std::cos(x * 2.0f * M_PI / period);
        float ny = std::cos(y * 2.0f * M_PI / period);
        float nz = std::cos(z * 2.0f * M_PI / period);
        float nw = std::sin(x * 2.0f * M_PI / period);
        float nv = std::sin(y * 2.0f * M_PI / period);
        float nu = std::sin(z * 2.0f * M_PI / period);
        return noise.GetNoise(nx, ny, nz);
    }

    float mirror(float coord, float maxCoord) {
        return maxCoord - std::abs(std::fmod(coord, maxCoord) - maxCoord);
    }

    float getMirroredNoise(float x, float y, float z, float tileSize){
        float mx = mirror(x, tileSize);
        float my = mirror(y, tileSize);
        float mz = mirror(z, tileSize);
        return noise.GetNoise(mx, my, mz);
    }
};

// Multi-Channel Noise System with Separate Settings per Channel
class MultiChannelNoise3D {
private:
    struct ChannelSettings {
        FastNoiseLite::NoiseType noiseType;
        int seed;
        float frequency;
    };

    std::vector<std::unique_ptr<Noise3D>> noiseChannels;  // Store noise for each channel - now using base class
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
            noiseChannels.push_back(std::make_unique<GenericNoise3D>(setting.noiseType, setting.seed, setting.frequency)); // Use GenericNoise3D
        }
        channelSettings = settings; // Store settings for potential later use or inspection
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
        Hammock::Logger::log(Hammock::LOG_LEVEL_DEBUG, "Generating multichannel noise ... \n");
        float min = 1.0, max = 0.0;
        float* buffer = new float[width * height * depth * numChannels];  // Allocate for variable channels

        int idx = 0;
        for (int z = 0; z < depth; ++z) {
            for (int y = 0; y < height; ++y) {
                for (int x = 0; x < width; ++x) {
                    // Generate noise values for each channel and scale them
                    for (int c = 0; c < numChannels; ++c) {
                        float noiseValue = noiseChannels[c]->getMirroredNoise(x,y,z, 128.0f);
                        float scaledValue = scaleValue(noiseValue); // Scale to the defined range
                        buffer[idx++] = scaledValue;

                        if (scaledValue > max) {
                            max = scaledValue;
                        }
                        if (scaledValue < min) {
                            min = scaledValue;
                        }
                    }
                }
            }
        }

        Hammock::Logger::log(Hammock::LOG_LEVEL_DEBUG, "DONE\n");
        Hammock::Logger::log(Hammock::LOG_LEVEL_DEBUG, "Min %f, Max %f\n", min, max);
        return buffer;  // Caller is responsible for deleting the buffer
    }

     int getNumChannels() const {
        return numChannels;
    }

    const ChannelSettings& getChannelSettings(int channelIndex) const {
        if (channelIndex >= 0 && channelIndex < numChannels) {
            return channelSettings[channelIndex];
        }
        static ChannelSettings defaultSettings = {FastNoiseLite::NoiseType_OpenSimplex2, 0, 0.0f}; // Return default if index is out of range
        return defaultSettings;
    }
};

#endif
