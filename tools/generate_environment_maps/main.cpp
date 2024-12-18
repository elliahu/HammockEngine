#include <iostream>

#include "core/HmckDevice.h"
#include "core/HmckResourceManager.h"
#include "io/HmckWindow.h"
#include "resources/HmckGenerator.h"
#include "utils/HmckScopedMemory.h"
#include "utils/HmckUtils.h"

#define DIM 512

int main(int argc, char * argv[]) {
    // create resources
    // unable to run headless as of now, so will be using a flash window
    Hmck::Window window{1, 1, "Generating Environment Maps"};
    Hmck::VulkanInstance instance{};
    window.createWindowSurface(instance);
    Hmck::Device device{instance, window.getSurface()};
    Hmck::DeviceStorage resources{device};
    Hmck::Generator environment;

    // get the source hdr file
    std::string hdrFilename(argv[1]); // first argument
    if(!Hmck::Filesystem::fileExists(hdrFilename)) {
        std::cerr << "File does not exist " << hdrFilename << std::endl;
        return EXIT_FAILURE;
    }

    // get the irradiance map destination
    std::string irradianceFilename(argv[2]);

    // get the brdf lut map destination
    std::string brdflutFilename(argv[3]);

    std::cout << "Generating environment maps..." << std::endl;

    // load the source env and generate rest
    int32_t w,h,c;
    Hmck::ScopedMemory environmentData = Hmck::ScopedMemory(Hmck::Filesystem::readImage(hdrFilename,w,h,c, Hmck::Filesystem::ImageFormat::R32G32B32A32_SFLOAT));
    environment.readEnvironmentMap(environmentData.get(), sizeof(float), w,h,c, resources, VK_FORMAT_R32G32B32A32_SFLOAT);
    environment.generatePrefilteredMap(device, resources, VK_FORMAT_R32G32B32A32_SFLOAT);
    environment.generateIrradianceMap(device, resources, VK_FORMAT_R32G32B32A32_SFLOAT);
    environment.generateBRDFLookUpTable(device, resources, 512, VK_FORMAT_R32G32_SFLOAT);

    // Irradiance map
    {
        // create host visible image
        VkImageCreateInfo irradianceImageCreateInfo(Hmck::Init::imageCreateInfo());
        irradianceImageCreateInfo.imageType = VK_IMAGE_TYPE_2D;
        irradianceImageCreateInfo.format = VK_FORMAT_R32G32B32A32_SFLOAT;
        irradianceImageCreateInfo.extent.width = w;
        irradianceImageCreateInfo.extent.height = h;
        irradianceImageCreateInfo.extent.depth = 1;
        irradianceImageCreateInfo.arrayLayers = 1;
        irradianceImageCreateInfo.mipLevels = 1;
        irradianceImageCreateInfo.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;
        irradianceImageCreateInfo.samples = VK_SAMPLE_COUNT_1_BIT;
        irradianceImageCreateInfo.tiling = VK_IMAGE_TILING_LINEAR;
        irradianceImageCreateInfo.usage = VK_IMAGE_USAGE_TRANSFER_DST_BIT;

        VkImage dstIrradianceImage;
        VkDeviceMemory dstIrradianceImageMemory;
        device.createImageWithInfo(irradianceImageCreateInfo, VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT, dstIrradianceImage, dstIrradianceImageMemory);

        // copy on-device image contents to host-visible image
        device.copyImageToHostVisibleImage(resources.getTexture2D(environment.irradianceMap)->image,dstIrradianceImage,w,h);

        // map memory
        const float * irradianceImageData;
        // Get layout of the image (including row pitch)
        VkImageSubresource subResource{};
        subResource.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
        VkSubresourceLayout subResourceLayout;
        vkGetImageSubresourceLayout(device.device(), dstIrradianceImage, &subResource, &subResourceLayout);
        // Map image memory so we can start copying from it
        vkMapMemory(device.device(), dstIrradianceImageMemory, 0, VK_WHOLE_SIZE, 0, (void**)&irradianceImageData);
        irradianceImageData += subResourceLayout.offset;


        // write the image
        Hmck::Filesystem::writeImage(irradianceFilename, irradianceImageData, sizeof(float), w,h, 4, Hmck::Filesystem::WriteImageDefinition::HDR);

        // clean
        vkUnmapMemory(device.device(), dstIrradianceImageMemory);
        vkFreeMemory(device.device(), dstIrradianceImageMemory, nullptr);
        vkDestroyImage(device.device(), dstIrradianceImage, nullptr);

        std::cout << "Irradiance map created at " << irradianceFilename << std::endl;
    }

    // BRDF lut
    {
        // create host visible image
        VkImageCreateInfo brdflutImageCreateInfo(Hmck::Init::imageCreateInfo());
        brdflutImageCreateInfo.imageType = VK_IMAGE_TYPE_2D;
        brdflutImageCreateInfo.format = VK_FORMAT_R32G32_SFLOAT;
        brdflutImageCreateInfo.extent.width = DIM;
        brdflutImageCreateInfo.extent.height = DIM;
        brdflutImageCreateInfo.extent.depth = 1;
        brdflutImageCreateInfo.arrayLayers = 1;
        brdflutImageCreateInfo.mipLevels = 1;
        brdflutImageCreateInfo.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;
        brdflutImageCreateInfo.samples = VK_SAMPLE_COUNT_1_BIT;
        brdflutImageCreateInfo.tiling = VK_IMAGE_TILING_LINEAR;
        brdflutImageCreateInfo.usage = VK_IMAGE_USAGE_TRANSFER_DST_BIT;

        VkImage dstBrdflutImage;
        VkDeviceMemory dstBrdflutImageMemory;
        device.createImageWithInfo(brdflutImageCreateInfo, VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT, dstBrdflutImage, dstBrdflutImageMemory);

        // copy on-device image contents to host-visible image
        device.copyImageToHostVisibleImage(resources.getTexture2D(environment.brdfLookUpTable)->image,dstBrdflutImage,DIM,DIM);

        // map memory
        const float * brdflutImageData;
        // Get layout of the image (including row pitch)
        VkImageSubresource subResource{};
        subResource.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
        VkSubresourceLayout subResourceLayout;
        vkGetImageSubresourceLayout(device.device(), dstBrdflutImage, &subResource, &subResourceLayout);
        // Map image memory so we can start copying from it
        vkMapMemory(device.device(), dstBrdflutImageMemory, 0, VK_WHOLE_SIZE, 0, (void**)&brdflutImageData);
        brdflutImageData += subResourceLayout.offset;

        // RG to RGB
        std::vector<float> hdrData(DIM * DIM * 3); // Allocate buffer for 3 channels

        for (uint32_t y = 0; y < DIM; ++y) {
            for (uint32_t x = 0; x < DIM; ++x) {
                size_t srcIndex = (y * DIM + x) * 2; // 2 channels in source data
                size_t dstIndex = (y * DIM + x) * 3; // 3 channels in destination data

                // Copy R and G, set B to 0
                hdrData[dstIndex + 0] = brdflutImageData[srcIndex + 0];  // R
                hdrData[dstIndex + 1] = brdflutImageData[srcIndex + 1];  // G
                hdrData[dstIndex + 2] = 0.0f;                               // B
            }
        }

        // write the image
        Hmck::Filesystem::writeImage(brdflutFilename, hdrData.data(), sizeof(float), DIM,DIM, 3, Hmck::Filesystem::WriteImageDefinition::HDR);

        // clean
        vkUnmapMemory(device.device(), dstBrdflutImageMemory);
        vkFreeMemory(device.device(), dstBrdflutImageMemory, nullptr);
        vkDestroyImage(device.device(), dstBrdflutImage, nullptr);

        std::cout << "BRDF Look-up table created at " << brdflutFilename << std::endl;
    }


    // cleanup
    environment.destroy(resources);
    vkDestroySurfaceKHR(instance.getInstance(), window.getSurface(), nullptr);

    // end
    return EXIT_SUCCESS;
}
