#include <iostream>
#include <cstdint>
#include "CloudRenderer.h"



int main(int argc, char * argv[]) {
    Hammock::ArgParser parser;

    parser.addArgument<int32_t>("width", "Window width in pixels");
    parser.addArgument<int32_t>("height", "Window height in pixels");

    try {
        parser.parse(argc, argv);
    } catch (const std::exception & e) {
        Hammock::Logger::log(Hammock::LOG_LEVEL_ERROR, e.what());
    }

    const int32_t width = parser.get<int32_t>("width");
    const int32_t height = parser.get<int32_t>("height");

    CloudRenderer renderer(width, height);
    renderer.run();
}
