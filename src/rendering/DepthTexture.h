#pragma once

#include <vulkan/vulkan.h>
#include <vector>

namespace vecs {

	// Forward Declarations
	class Device;

	class DepthTexture {
	public:
		VkImage image;
		VkFormat format;
		VkDeviceMemory deviceMemory;
		VkImageView view;
		uint32_t width, height;
		VkDescriptorImageInfo descriptor;

		void init(Device* device, VkExtent2D extent);

		void cleanup();

	private:
		Device* device;

		void createImage(VkFormat format, VkImageUsageFlags usageFlags);

		VkFormat findSupportedFormat(const std::vector<VkFormat>& candidates, VkImageTiling tiling, VkFormatFeatureFlags features);
		bool hasStencilComponent(VkFormat format);
	};
}
