#pragma once

#include <cassert>
#include <functional>
#include <filesystem>
#include <fstream>
#include <iostream>
#include <string>
#include <cmath>
#include <stb_image.h>
#include <stb_image_write.h>

#include "hammock/utils/Logger.h"

namespace hammock{
    namespace Filesystem {
        inline bool fileExists(const std::string &filename) {
            std::ifstream f(filename.c_str());
            return !f.fail();
        }

        inline std::vector<char> readFile(const std::string &filePath) {
            std::ifstream file{filePath, std::ios::ate | std::ios::binary};

            if (!file.is_open()) {
                throw std::runtime_error("failed to open file: " + filePath);
            }

            size_t fileSize = static_cast<size_t>(file.tellg());
            std::vector<char> buffer(fileSize);

            file.seekg(0);
            file.read(buffer.data(), fileSize);

            file.close();
            return buffer;
        }

        inline void dump(const std::string &filename, const std::string &data) {
            std::ofstream outFile(filename);
            if (outFile.is_open()) {
                outFile << data;
                outFile.close();
            } else {
                throw std::runtime_error("could not dump into file!");
            }
        }

        inline std::vector<std::string> ls(const std::string &directoryPath) {
            std::vector<std::string> fileList;

            try {
                for (const auto &entry: std::filesystem::directory_iterator(directoryPath)) {
                    if (entry.is_regular_file()) {
                        // Check if it's a regular file
                        fileList.push_back(entry.path().string());
                    }
                }
            } catch (const std::filesystem::filesystem_error &e) {
                Logger::log(LOG_LEVEL_ERROR, "Error: Error accessing directory %s\n", directoryPath.c_str());
                throw std::runtime_error("Error: Error accessing directory");
            }

            std::sort(fileList.begin(), fileList.end());
            return fileList;
        }

        enum class WriteImageDefinition {
            SDR, HDR
        };

        inline void writeImage(const std::string &filename, const void *buffer, uint32_t instanceSize, uint32_t width,
                               uint32_t height, uint32_t channels,
                               WriteImageDefinition definition = WriteImageDefinition::SDR) {
            // Handle SDR and HDR separately
            if (definition == WriteImageDefinition::SDR) {
                // For SDR, normalize to 8-bit and save as PNG
                unsigned char *normalizedBuffer = nullptr;
                if (instanceSize > 1) {
                    normalizedBuffer = new unsigned char[width * height * channels];
                    const float *floatBuffer = static_cast<const float *>(buffer);
                    for (size_t i = 0; i < width * height * channels; ++i) {
                        normalizedBuffer[i] = static_cast<unsigned char>(std::clamp(
                            floatBuffer[i] * 255.0f, 0.0f, 255.0f));
                    }
                } else {
                    // If instance size is already 1 byte, cast the buffer
                    normalizedBuffer = const_cast<unsigned char *>(static_cast<const unsigned char *>(buffer));
                }

                // Use stb_image_write to save the image as PNG
                int success = stbi_write_png(filename.c_str(), width, height, channels, normalizedBuffer,
                                             width * channels);
                if (!success) {
                    Logger::log(LOG_LEVEL_ERROR, "Error: Failed to save SDR image!");
                    throw std::runtime_error("Failed to save image: " + filename);
                }

                // Clean up if normalization was done
                if (instanceSize > 1) {
                    delete[] normalizedBuffer;
                }
            } else if (definition == WriteImageDefinition::HDR) {
                // For HDR, save as HDR file using float buffer directly
                if (instanceSize != sizeof(float)) {
                    Logger::log(LOG_LEVEL_ERROR, "Error: HDR requires float data!");
                    throw std::runtime_error("HDR images require buffer with float data.");
                }

                const float *floatBuffer = static_cast<const float *>(buffer);
                int success = stbi_write_hdr(filename.c_str(), width, height, channels, floatBuffer);
                if (!success) {
                    Logger::log(LOG_LEVEL_ERROR, "Error: Failed to save HDR image!");
                    throw std::runtime_error("Failed to save HDR image: " + filename);
                }
            } else {
                Logger::log(LOG_LEVEL_ERROR, "Error: Unknown image definition!");
                throw std::invalid_argument("Unknown image definition specified.");
            }
        }

        enum ReadImageLoadingFlags {
            FLIP_Y = 0x00000001
        };

        enum class ImageFormat {
            R32_SFLOAT,
            R32G32_SFLOAT,
            R32G32B32_SFLOAT,
            R32G32B32A32_SFLOAT,
            R8_UNORM,
            R8G8_UNORM,
            R8G8B8_UNORM,
            R8G8B8A8_UNORM
        };

        inline const void *readImage(const std::string &filename, int &width, int &height, int &channels,
                                     const ImageFormat format = ImageFormat::R32G32B32A32_SFLOAT, uint32_t flags = 0) {
            int desiredChannels = 4; // Default desired channels
            if (format == ImageFormat::R32_SFLOAT || format == ImageFormat::R8_UNORM)
                desiredChannels = 1;
            else if (format == ImageFormat::R32G32_SFLOAT || format == ImageFormat::R8G8_UNORM)
                desiredChannels = 2;
            else if (format == ImageFormat::R32G32B32_SFLOAT || format == ImageFormat::R8G8B8_UNORM)
                desiredChannels = 3;
            else if (format == ImageFormat::R32G32B32A32_SFLOAT || format == ImageFormat::R8G8B8A8_UNORM)
                desiredChannels = 4;

            stbi_set_flip_vertically_on_load(flags & FLIP_Y); // Handle vertical flipping if requested

            // Load HDR data
            if (format == ImageFormat::R32_SFLOAT || format == ImageFormat::R32G32_SFLOAT ||
                format == ImageFormat::R32G32B32_SFLOAT || format == ImageFormat::R32G32B32A32_SFLOAT) {
                int hdrChannels = 4; // HDR always has 4 components
                const float *data = stbi_loadf(filename.c_str(), &width, &height, &channels, hdrChannels);
                stbi_set_flip_vertically_on_load(false); // Reset flipping after loading

                if (!data) {
                    Logger::log(LOG_LEVEL_ERROR, "Error: Failed to load image!\n");
                    throw std::runtime_error("Image loading failed.");
                }

                size_t pixelCount = static_cast<size_t>(width) * static_cast<size_t>(height);

                // Allocate memory for the desired component count
                float *processedData = new float[pixelCount * desiredChannels];

                for (size_t i = 0; i < pixelCount; ++i) {
                    // Copy the appropriate number of components based on the desired format
                    if (desiredChannels == 1) {
                        // R32_SFLOAT
                        processedData[i] = data[i * 4 + 0]; // Copy R
                    } else if (desiredChannels == 2) {
                        // R32G32_SFLOAT
                        processedData[i * 2 + 0] = data[i * 4 + 0]; // Copy R
                        processedData[i * 2 + 1] = data[i * 4 + 1]; // Copy G
                    } else if (desiredChannels == 3) {
                        // R32G32B32_SFLOAT
                        processedData[i * 3 + 0] = data[i * 4 + 0]; // Copy R
                        processedData[i * 3 + 1] = data[i * 4 + 1]; // Copy G
                        processedData[i * 3 + 2] = data[i * 4 + 2]; // Copy B
                    } else {
                        // R32G32B32A32_SFLOAT
                        processedData[i * 4 + 0] = data[i * 4 + 0]; // Copy R
                        processedData[i * 4 + 1] = data[i * 4 + 1]; // Copy G
                        processedData[i * 4 + 2] = data[i * 4 + 2]; // Copy B
                        processedData[i * 4 + 3] = data[i * 4 + 3]; // Copy A
                    }
                }

                stbi_image_free(const_cast<float *>(data)); // Free the original 4-component data
                channels = desiredChannels; // Update channel count
                return processedData; // Return processed data
            }

            // Load SDR data
            if (format == ImageFormat::R8_UNORM || format == ImageFormat::R8G8_UNORM ||
                format == ImageFormat::R8G8B8_UNORM || format == ImageFormat::R8G8B8A8_UNORM) {
                const unsigned char *data = stbi_load(filename.c_str(), &width, &height, &channels, 4);
                // Always load as 4 components
                stbi_set_flip_vertically_on_load(false); // Reset flipping after loading

                if (!data) {
                    Logger::log(LOG_LEVEL_ERROR, "Error: Failed to load image!\n");
                    throw std::runtime_error("Image loading failed.");
                }

                size_t pixelCount = static_cast<size_t>(width) * static_cast<size_t>(height);
                unsigned char *processedData = new unsigned char[pixelCount * desiredChannels];

                for (size_t i = 0; i < pixelCount; ++i) {
                    if (desiredChannels == 1) {
                        // R8_UNORM
                        processedData[i] = data[i * 4 + 0]; // Copy R
                    } else if (desiredChannels == 2) {
                        // R8G8_UNORM
                        processedData[i * 2 + 0] = data[i * 4 + 0]; // Copy R
                        processedData[i * 2 + 1] = data[i * 4 + 1]; // Copy G
                    } else if (desiredChannels == 3) {
                        // R8G8B8_UNORM
                        processedData[i * 3 + 0] = data[i * 4 + 0]; // Copy R
                        processedData[i * 3 + 1] = data[i * 4 + 1]; // Copy G
                        processedData[i * 3 + 2] = data[i * 4 + 2]; // Copy B
                    } else {
                        // R8G8B8A8_UNORM
                        processedData[i * 4 + 0] = data[i * 4 + 0]; // Copy R
                        processedData[i * 4 + 1] = data[i * 4 + 1]; // Copy G
                        processedData[i * 4 + 2] = data[i * 4 + 2]; // Copy B
                        processedData[i * 4 + 3] = data[i * 4 + 3]; // Copy A
                    }
                }

                stbi_image_free(const_cast<unsigned char *>(data)); // Free original 4-component data
                channels = desiredChannels; // Update channel count
                return processedData; // Return processed data
            }

            Logger::log(LOG_LEVEL_ERROR, "Error: Reached unreachable code path in readImage\n");
            throw std::runtime_error("Failed to load image!");
        }


        // Can also be used to read cube map faces
        inline const float *readVolume(const std::vector<std::string> &slices, int &width, int &height, int &channels,
                                       int &depth, const ImageFormat format = ImageFormat::R32G32B32A32_SFLOAT,
                                       uint32_t flags = 0) {
            if (slices.empty()) {
                Logger::log(LOG_LEVEL_ERROR, "Error: No slice file paths provided\n");
                throw std::runtime_error("Error: No slice file paths provided.");
            }
            // TODO make this general to support uint buffer as well as float buffers
            if (format == ImageFormat::R8_UNORM || format == ImageFormat::R8G8B8_UNORM || format ==
                ImageFormat::R8G8B8A8_UNORM) {
                Logger::log(LOG_LEVEL_ERROR, "Error: SDR content not supported\n");
                throw std::runtime_error("Error: SDR content not supported.");
            }

            // Read the first slice to determine width, height, and channels
            const float *firstSlice = static_cast<const float *>(readImage(
                slices[0], width, height, channels, format, flags));
            depth = slices.size(); // The number of slices determines the depth

            // Allocate memory for the 3D texture
            size_t sliceSize = width * height * channels;
            float *volumeData = new float[sliceSize * depth];

            // Copy the first slice into the buffer
            std::copy(firstSlice, firstSlice + sliceSize, volumeData);
            stbi_image_free(const_cast<float *>(firstSlice)); // Free the first slice

            // Read and copy the remaining slices
            for (size_t i = 1; i < slices.size(); ++i) {
                int currentWidth, currentHeight, currentChannels;
                const float *sliceData = static_cast<const float *>(readImage(
                    slices[i], currentWidth, currentHeight, currentChannels, format, flags));

                // Validate dimensions match
                if (currentWidth != width || currentHeight != height || currentChannels != channels) {
                    delete[] volumeData;
                    stbi_image_free(const_cast<float *>(sliceData));
                    Logger::log(LOG_LEVEL_ERROR, "Error: Slice dimensions or channels mismatch in slice %d\n", i);
                    throw std::runtime_error("Error: Slice dimensions or channels mismatch!");
                }

                // Copy the slice into the correct position in the 3D buffer
                std::copy(sliceData, sliceData + sliceSize, volumeData + i * sliceSize);
                stbi_image_free(const_cast<float *>(sliceData)); // Free the current slice
            }

            return volumeData;
        }


    } // namespace Filesystem
}