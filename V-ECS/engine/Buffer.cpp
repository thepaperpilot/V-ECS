#include "Buffer.h"

using namespace vecs;

VkResult Buffer::map(VkDeviceSize size, VkDeviceSize offset) {
	return vkMapMemory(*device, memory, offset, size, 0, &mapped);
}

void Buffer::unmap() {
	if (mapped) {
		vkUnmapMemory(*device, memory);
		mapped = nullptr;
	}
}

void Buffer::copyTo(void* data, VkDeviceSize size) {
	if (mapped) memcpy(mapped, data, size);
	else {
		map(size);
		memcpy(mapped, data, size);
		unmap();
	}
}

void Buffer::cleanup() {
	if (buffer) vkDestroyBuffer(*device, buffer, nullptr);
	if (memory) vkFreeMemory(*device, memory, nullptr);
}
