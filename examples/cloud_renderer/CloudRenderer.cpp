#include "CloudRenderer.h"

CloudRenderer::CloudRenderer(const int32_t width, const int32_t height) :
window{instance, "Cloud Renderer", width, height},
device(instance, window.getSurface()) {

}

void CloudRenderer::run() {

    while (!window.shouldClose()) {
        window.pollEvents();
    }

    PerlinNoise3D noise;
    float * noiseData = noise.generateNoiseVolume(100,100,1);


    device.waitIdle();
}
