#pragma once

#include <string>
#include <filesystem>

namespace Hmck {
    class Shader {
    public:
        static std::filesystem::path getShaderDirectory() {
            return std::filesystem::path(__FILE__).parent_path();
        }

        static std::filesystem::path getCompiledShaderPath(const std::string &filename) {
            // Get the directory of the current source file
            const std::filesystem::path sourceDir = getShaderDirectory();
            return sourceDir / "compiled" / filename;
        }

        static std::filesystem::path getRawShaderPath(const std::string &filename) {
            // Get the directory of the current source file
            const std::filesystem::path sourceDir = getShaderDirectory();
            return sourceDir / "raw" / filename;
        }

        static std::filesystem::path getRootShaderPath(const std::string &filename) {
            // Get the directory of the current source file
            const std::filesystem::path sourceDir = getShaderDirectory();
            return sourceDir / filename;
        }
    };
}
