#include <iostream>
#include <cstdint>
#include "scenes/ParticipatingMediumTestScene.h"
#include "scenes/CloudBoundingBoxTestScene.h"


int main(int argc, char * argv[]) {
    ArgParser parser;
    parser.addArgument<int32_t>("width", "Window width in pixels");
    parser.addArgument<int32_t>("height", "Window height in pixels");
    parser.addArgument<std::string>("scene", "Scene option: [medium, clouds]");

    try {
        parser.parse(argc, argv);
    } catch (const std::exception & e) {
        Logger::log(LOG_LEVEL_ERROR, e.what());
    }

    const auto width = parser.get<int32_t>("width");
    const auto height = parser.get<int32_t>("height");
    auto scene = parser.get<std::string>("scene");

    if (scene == "medium") {
        ParticipatingMediumTestScene s(width, height);
        s.run();
    }
    else if (scene == "clouds") {
        CloudBoundingBoxTestScene s(width, height);
        s.run();
    }
    else {
        Logger::log(LOG_LEVEL_ERROR, "Invalid scene option");
    }
}
