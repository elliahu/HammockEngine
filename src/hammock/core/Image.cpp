#include "hammock/core/Image.h"

void hammock::rendergraph::Image::load() {
    // TODO Implement Vulkan image creation here
    // 1. Create the VkImage
    // 2. Allocate memory
    // 3. Create image view
    resident = true;
}

void hammock::rendergraph::Image::unload() {
    // TODO Clean up Vulkan resources
    resident = false;
}

void hammock::rendergraph::SampledImage::load() {
    // TODO Implement Vulkan image creation here
    // 1. Create the VkImage
    // 2. Allocate memory
    // 3. Create image view
    resident = true;
}

void hammock::rendergraph::SampledImage::unload() {
    // TODO Clean up Vulkan resources
    resident = false;
}
