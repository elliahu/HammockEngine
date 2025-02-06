#ifndef NOISE3D_H
#define NOISE3D_H

#include <vector>
#include <functional>
#include <random>
#include <cmath>
#include <algorithm>
#include <thread>
#include <mutex>

struct WorleyNoiseSettings {
    int seed = 42;
    int featurePoints = 16;
    bool invert = true;
};

class Noise3D {
public:
    Noise3D(int width, int height, int depth, int channels = 1)
        : width(width), height(height), depth(depth),
          channels(std::min(channels, 4)),
          noiseData(width * height * depth * this->channels) {}

    void generateWorleyNoise(const std::vector<WorleyNoiseSettings>& settings) {
        if (settings.size() != static_cast<size_t>(channels)) {
            throw std::runtime_error("Number of settings must match channels.");
        }

        std::vector<std::thread> threads;
        for (int c = 0; c < channels; ++c) {
            threads.emplace_back(&Noise3D::generateChannelWorley, this, c, settings[c]);
        }

        for (auto& thread : threads) {
            thread.join();
        }
    }

    const std::vector<float>& getData() const { return noiseData; }

private:
    int width, height, depth, channels;
    std::vector<float> noiseData;
    std::mutex noiseMutex;

    void generateChannelWorley(int channel, const WorleyNoiseSettings& settings) {
        std::mt19937 rng(settings.seed);
        std::uniform_real_distribution<float> dist(0.0f, 1.0f);

        // Precompute feature points as vector of 3D points
        std::vector<std::tuple<float, float, float>> featurePoints;
        featurePoints.reserve(settings.featurePoints);
        for (int i = 0; i < settings.featurePoints; ++i) {
            featurePoints.emplace_back(dist(rng), dist(rng), dist(rng));
        }

        // Parallel processing of noise volume
        int threadCount = std::thread::hardware_concurrency();
        std::vector<std::thread> sliceThreads;
        int slicesPerThread = depth / threadCount;

        auto processSliceRange = [&](int startZ, int endZ) {
            std::vector<float> localChannelNoise;
            float localMinDist = std::numeric_limits<float>::max();
            float localMaxDist = std::numeric_limits<float>::lowest();

            // Compute distances for this slice range
            for (int z = startZ; z < endZ; ++z) {
                for (int y = 0; y < height; ++y) {
                    for (int x = 0; x < width; ++x) {
                        float minDist = 1e6f;
                        float normX = static_cast<float>(x) / width;
                        float normY = static_cast<float>(y) / height;
                        float normZ = static_cast<float>(z) / depth;

                        for (const auto& fp : featurePoints) {
                            for (int ox = -1; ox <= 1; ++ox) {
                                for (int oy = -1; oy <= 1; ++oy) {
                                    for (int oz = -1; oz <= 1; ++oz) {
                                        float dx = normX - (std::get<0>(fp) + ox);
                                        float dy = normY - (std::get<1>(fp) + oy);
                                        float dz = normZ - (std::get<2>(fp) + oz);
                                        float distSq = dx * dx + dy * dy + dz * dz;
                                        minDist = std::min(minDist, std::sqrt(distSq));
                                    }
                                }
                            }
                        }

                        localChannelNoise.push_back(minDist);
                        localMinDist = std::min(localMinDist, minDist);
                        localMaxDist = std::max(localMaxDist, minDist);
                    }
                }
            }

            // Thread-safe update of global results
            std::lock_guard<std::mutex> lock(noiseMutex);
            for (size_t i = 0; i < localChannelNoise.size(); ++i) {
                float normalized = (localChannelNoise[i] - localMinDist) /
                                   (localMaxDist - localMinDist);

                if (settings.invert) {
                    normalized = 1.0f - normalized;
                }

                size_t globalIndex = ((startZ * height * width + i) * channels) + channel;
                noiseData[globalIndex] = normalized;
            }
        };

        // Distribute Z slices across threads
        for (int t = 0; t < threadCount; ++t) {
            int startZ = t * slicesPerThread;
            int endZ = (t == threadCount - 1) ? depth : startZ + slicesPerThread;
            sliceThreads.emplace_back(processSliceRange, startZ, endZ);
        }

        // Wait for all slice threads to complete
        for (auto& thread : sliceThreads) {
            thread.join();
        }
    }
};

#endif