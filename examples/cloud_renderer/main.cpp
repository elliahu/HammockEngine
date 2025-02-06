#include <iostream>
#include <cstdint>
#include "scenes/ParticipatingMediumTestScene.h"
#include "scenes/CloudBoundingBoxTestScene.h"


int main(int argc, char * argv[]) {
    ArgParser parser;
    parser.addArgument<int32_t>("width", "Window width in pixels");
    parser.addArgument<int32_t>("height", "Window height in pixels");

    try {
        parser.parse(argc, argv);
    } catch (const std::exception & e) {
        Logger::log(LOG_LEVEL_ERROR, e.what());
    }

    const int32_t width = parser.get<int32_t>("width");
    const int32_t height = parser.get<int32_t>("height");

    CloudBoundingBoxTestScene scene(width, height);
    scene.run();
}
