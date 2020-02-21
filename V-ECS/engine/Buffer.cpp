#include "Buffer.h"

using namespace vecs;

void* Buffer::map(VkDeviceSize size, VkDeviceSize offset) {
	if (vkMapMemory(*device, memory, offset, size, 0, &mapped) != VK_SUCCESS) {
		throw std::runtime_error("failed to map memory!");
	}
	return mapped;
}

void Buffer::unmap() {
	if (mapped) {
		vkUnmapMemory(*device, memory);
		mapped = nullptr;
	}
}

void Buffer::copyTo(void* data, VkDeviceSize size) {
	if (mapped) {
		memcpy(mapped, data, size);
	}
	else {
		map(size);
		memcpy(mapped, data, size);
		unmap();
	}
}

void Buffer::copyTo(void* position, void* data, VkDeviceSize size) {
	if (mapped) {
		memcpy(position, data, size);
	}
}

void Buffer::cleanup() {
	if (buffer) vkDestroyBuffer(*device, buffer, nullptr);
	if (memory) vkFreeMemory(*device, memory, nullptr);
}
