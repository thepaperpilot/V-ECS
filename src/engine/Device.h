#pragma once

#include "Buffer.h"

#include <vector>
#include <optional>

namespace vecs {

	// List of physical device extensions that must be supported for a physical
	// device to be considered at all suitable
	static const char* const deviceExtensions[] {
		VK_KHR_SWAPCHAIN_EXTENSION_NAME
	};
	static constexpr size_t deviceExtensionsSize = sizeof(deviceExtensions) / sizeof(deviceExtensions[0]);
	
	struct QueueFamilyIndices {
		std::optional<uint32_t> graphics;
		std::optional<uint32_t> present;

		bool isComplete() {
			// Check that every family feature has some value
			return graphics.has_value() && present.has_value();
		}
	};

	struct SwapChainSupportDetails {
		VkSurfaceCapabilitiesKHR capabilities;
		std::vector<VkSurfaceFormatKHR> formats;
		std::vector<VkPresentModeKHR> presentModes;
	};

	struct PhysicalDeviceCandidate {
		VkPhysicalDevice physicalDevice;
		QueueFamilyIndices indices;
		SwapChainSupportDetails swapChainSupport;
	};
	
	class Device {
	public:
		VkPhysicalDevice physical;
		VkDevice logical;

		QueueFamilyIndices queueFamilyIndices;
		SwapChainSupportDetails swapChainSupport;

		VkCommandPool commandPool;

		operator VkDevice() { return logical; }

		Device(VkInstance instance, VkSurfaceKHR surface);

		VkCommandPool createCommandPool(uint32_t queueFamilyIndex, VkCommandPoolCreateFlags createFlags = VK_COMMAND_POOL_CREATE_RESET_COMMAND_BUFFER_BIT);

		Buffer createBuffer(VkDeviceSize size, VkBufferUsageFlags usage, VkMemoryPropertyFlags properties);
		void beginCommandBuffer(VkCommandBuffer buffer);
		void beginSecondaryCommandBuffer(VkCommandBuffer buffer, VkCommandBufferInheritanceInfo* info);
		void copyBuffer(Buffer* src, Buffer* dest, VkQueue queue, VkBufferCopy* copyRegion = nullptr);
		VkCommandBuffer createCommandBuffer(VkCommandBufferLevel level, VkCommandPool commandPool = nullptr, bool begin = true);
		std::vector<VkCommandBuffer> createCommandBuffers(VkCommandBufferLevel level, int amount, VkCommandPool commandPool = nullptr, bool begin = false);
		void submitCommandBuffer(VkCommandBuffer buffer, VkQueue queue, VkCommandPool commandPool = nullptr, bool free = true);

		uint32_t findMemoryType(uint32_t typeFilter, VkMemoryPropertyFlags properties);

		void cleanup();

	private:
		void pickPhysicalDevice(VkInstance instance, VkSurfaceKHR surface);
		QueueFamilyIndices findQueueFamilies(VkPhysicalDevice device, VkSurfaceKHR surface);
		SwapChainSupportDetails querySwapChainSupport(VkPhysicalDevice physicalDevice, VkSurfaceKHR surface);
		int rateDeviceSuitability(PhysicalDeviceCandidate candidate);
		bool checkDeviceExtensionSupport(VkPhysicalDevice device);

		void createLogicalDevice();
	};
}
