#pragma once

#include <vulkan/vulkan.h>

namespace vecs {

	// Forward Declarations
	class Device;

	class ThreadResources {
	public:
		VkCommandPool commandPool;
		VkQueue graphicsQueue;

		ThreadResources(Device* device, uint32_t queueIndex);

		void cleanup();

	private:
		Device* device;
	};
}
