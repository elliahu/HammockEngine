#include <iostream>
#include <cstdint>

#include <hammock/hammock.h>

constexpr uint32_t WINDOW_WIDTH = 1920;
constexpr uint32_t WINDOW_HEIGHT = 1080;

int main(int argc, const char * argv[]) {
    Hmck::VulkanInstance instance;
    Hmck::Window window{instance, "Cloud Renderer", WINDOW_WIDTH, WINDOW_HEIGHT};

}
