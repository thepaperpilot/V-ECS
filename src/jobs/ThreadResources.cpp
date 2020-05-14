#include "ThreadResources.h"

#include "../engine/Device.h"

using namespace vecs;

ThreadResources::ThreadResources(Device* device, uint32_t queueIndex) {
	this->device = device;
	commandPool = device->createCommandPool(device->queueFamilyIndices.graphics.value());
    vkGetDeviceQueue(*device, device->queueFamilyIndices.graphics.value(), queueIndex, &graphicsQueue);
}

void ThreadResources::cleanup() {
    // Destroy our command pool
    vkDestroyCommandPool(*device, commandPool, nullptr);
}
