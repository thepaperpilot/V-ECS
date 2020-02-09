#pragma once

#include "../ecs/World.h"

namespace vecs {

	// A component that stores the syncing objects and other data that may be
	// needed at different points of time during the rendering process
	struct RenderStateComponent : Component {

		// Configurations
		uint32_t maxFramesInFlight = 2;

		// State
		bool framebufferResized = false;
		size_t currentFrame = 0;
		uint32_t imageIndex;

		// Sync objects
		std::vector<VkSemaphore> imageAvailableSemaphores;
		std::vector<VkSemaphore> renderFinishedSemaphores;
		std::vector<VkFence> inFlightFences;
		std::vector<VkFence> imagesInFlight;

		void cleanup(VkDevice* device) override {
			for (size_t i = 0; i < maxFramesInFlight; i++) {
				vkDestroySemaphore(*device, renderFinishedSemaphores[i], nullptr);
				vkDestroySemaphore(*device, imageAvailableSemaphores[i], nullptr);
				vkDestroyFence(*device, inFlightFences[i], nullptr);
			}
		}
	};
}
