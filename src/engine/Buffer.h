#pragma once

// VMA will include vulkan.h itself
#include "../VulkanMemoryAllocator/src/vk_mem_alloc.h"

#ifndef VMA_NULL
   // Value used as null pointer. Define it to e.g.: nullptr, NULL, 0, (void*)0.
	#define VMA_NULL   nullptr
#endif

#include <stdexcept>

namespace vecs {

	// Forward Declarations
	class Device;

	class Buffer {
	public:
		VkDeviceSize size = 0;

		VkBuffer buffer = VK_NULL_HANDLE;
		VmaAllocation allocation = VMA_NULL;

		Buffer() {}

		Buffer(VmaAllocator allocator) : allocator(allocator) {}
		
		void* map(VkDeviceSize size = VK_WHOLE_SIZE, VkDeviceSize offset = 0);
		void unmap();
        void copyTo(void* data, VkDeviceSize size = VK_WHOLE_SIZE);
		void copyTo(void* position, void* data, VkDeviceSize size = VK_WHOLE_SIZE);

        operator VkBuffer() { return buffer; };

	private:
		VmaAllocator allocator = VMA_NULL;
		void* mapped = nullptr;
	};
}
