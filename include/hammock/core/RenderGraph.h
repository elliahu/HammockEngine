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
            std::vector<std::unique_ptr<RenderPass>> renderPasses;

            RenderGraph& addRenderPass(std::unique_ptr<RenderPass> renderPass) {
                renderPasses.emplace_back(std::move(renderPass));
                return *this;
            }

            void execute() {
                for (int i = 0; i < renderPasses.size(); i++) {
                    RenderPass& renderPass = *renderPasses[i];

                    Logger::log(LOG_LEVEL_DEBUG, "%d RENDER PASS %s\n", i, renderPass.debugName.c_str());


                    for (auto& read : renderPass.readsFrom) {

                        Logger::log(LOG_LEVEL_DEBUG, " - reads from resource %d\n", read);

                    }

                    for (auto& write : renderPass.writesTo) {

                        Logger::log(LOG_LEVEL_DEBUG, " - writes to resource %d\n", write);

                    }
                }
            }
        };

    }
};
