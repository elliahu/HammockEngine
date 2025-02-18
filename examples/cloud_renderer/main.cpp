#include <iostream>
#include <cstdint>
#include <hammock/hammock.h>

#include "scenes/NoiseEditor.h"
#include "scenes/SkyScene.h"

using namespace hammock;

int main(int argc, char * argv[]) {
    ArgParser parser;
    parser.addArgument<int32_t>("width", "Window width in pixels");
    parser.addArgument<int32_t>("height", "Window height in pixels");
    parser.addArgument<std::string>("scene", "Scene option: [noise, clouds]");

    try {
        parser.parse(argc, argv);
    } catch (const std::exception & e) {
        Logger::log(LOG_LEVEL_ERROR, e.what());
    }

    const auto width = parser.get<int32_t>("width");
    const auto height = parser.get<int32_t>("height");
    auto selectedScene = parser.get<std::string>("scene");

    if (selectedScene == "noise") {
        NoiseEditor editor{"Noise editor", 1024, 1024, 128, 128, 128};
        editor.render();
    }
    else if (selectedScene == "clouds") {
        SkyScene skyScene{"Sky rendering demo", static_cast<uint32_t>(width), static_cast<uint32_t>(height)};
        skyScene.render();
    }
    else {
        Logger::log(LOG_LEVEL_ERROR, "Invalid scene option");
    }
}