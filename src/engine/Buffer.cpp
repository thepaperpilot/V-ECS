#include "Buffer.h"

#include "Debugger.h"

using namespace vecs;

void* Buffer::map(VkDeviceSize size, VkDeviceSize offset) {
	VK_CHECK_RESULT(vmaMapMemory(allocator, allocation, &mapped));
	return mapped;
}

void Buffer::unmap() {
	if (mapped) {
		vmaUnmapMemory(allocator, allocation);
		mapped = nullptr;
	}
}

void Buffer::copyTo(void* data, VkDeviceSize size) {
	if (mapped) {
		memcpy(mapped, data, size);
	} else {
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
