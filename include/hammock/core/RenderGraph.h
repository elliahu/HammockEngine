#pragma once

#include <hammock/core/RenderPass.h>
#include <hammock/core/Resource.h>
#include <hammock/core/Device.h>
#include <hammock/resources/Buffer.h>
#include <hammock/utils/Logger.h>
#include <hammock/core/SwapChain.h>
#include <cassert>

#include "RenderPass.h"

namespace hammock {
    namespace rendergraph {



        class RenderGraph {
          public:

            ~RenderGraph() {
                renderPasses.clear();
            }

            std::vector<std::unique_ptr<RenderPass>> renderPasses;

            RenderGraph& addRenderPass(std::unique_ptr<RenderPass> renderPass) {
                renderPasses.emplace_back(std::move(renderPass));
                return *this;
            }

            void execute() {
                for (int i = 0; i < renderPasses.size(); i++) {
                    RenderPass& renderPass = *renderPasses[i];

                    Logger::log(LOG_LEVEL_DEBUG, "%d RENDER PASS %s\n", i, renderPass.debugName.c_str());


                    for (auto& read : renderPass.colorAttachmentReads) {
                        Logger::log(LOG_LEVEL_DEBUG, " - reads from attachment %d\n", read.first );
                    }
                    for (auto& read : renderPass.depthStencilAttachmentReads) {
                        Logger::log(LOG_LEVEL_DEBUG, " - reads from depth stencil %d\n", read.first );
                    }
                    for (auto& read : renderPass.textureReads) {
                        Logger::log(LOG_LEVEL_DEBUG, " - reads from texture %d\n", read.first );
                    }
                    for (auto& read : renderPass.bufferReads) {
                        Logger::log(LOG_LEVEL_DEBUG, " - reads from buffer %d\n", read.first );
                    }

                    for (auto& read : renderPass.colorAttachmentWrites) {
                        Logger::log(LOG_LEVEL_DEBUG, " - writes to attachment %d\n", read.first );
                    }
                    for (auto& read : renderPass.depthStencilAttachmentWrites) {
                        Logger::log(LOG_LEVEL_DEBUG, " - writes to  depth stencil %d\n", read.first );
                    }
                    for (auto& read : renderPass.textureWrites) {
                        Logger::log(LOG_LEVEL_DEBUG, " - writes to  texture %d\n", read.first );
                    }
                    for (auto& read : renderPass.bufferWrites) {
                        Logger::log(LOG_LEVEL_DEBUG, " - writes to  buffer %d\n", read.first );
                    }
                }
            }
        };

    }
};
