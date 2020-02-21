#pragma once

#include <vulkan/vulkan.h>
#include <stdexcept>

namespace vecs {

	// Forward Declarations
	class Device;

	class Buffer {
	public:
		VkDeviceSize size;
        VkBufferUsageFlags usageFlags;
        VkMemoryPropertyFlags propertiesFlags;

		VkBuffer buffer = VK_NULL_HANDLE;
		VkDeviceMemory memory = VK_NULL_HANDLE;

		Buffer() {}

		Buffer(VkDevice* device, VkDeviceSize size, VkBufferUsageFlags usageFlags, VkMemoryPropertyFlags propertiesFlags) {
			init(device, size, usageFlags, propertiesFlags);
		}

		void init(VkDevice* device, VkDeviceSize size, VkBufferUsageFlags usageFlags, VkMemoryPropertyFlags propertiesFlags) {
			this->device = device;
			this->size = size;
			this->usageFlags = usageFlags;
			this->propertiesFlags = propertiesFlags;
		}
		
		void* map(VkDeviceSize size = VK_WHOLE_SIZE, VkDeviceSize offset = 0);
		void unmap();
        void copyTo(void* data, VkDeviceSize size);
		void copyTo(void* position, void* data, VkDeviceSize size);
        void cleanup();

        operator VkBuffer() { return buffer; };

	private:
		VkDevice* device;

		void* mapped = nullptr;
	};
}
