#pragma once

#include "../engine/Buffer.h"
#include "../engine/Device.h"

#include <vulkan/vulkan.h>

namespace vecs {

	class DepthTexture {
	public:
		VkImage image;
		VkFormat format;
		VkDeviceMemory deviceMemory;
		VkImageView view;
		uint32_t width, height;
		VkDescriptorImageInfo descriptor;

		void init(Device* device, VkQueue copyQueue, VkExtent2D extent);

		void cleanup();

	private:
		Device* device;

		void createImage(VkFormat format, VkImageUsageFlags usageFlags);

		VkFormat findSupportedFormat(const std::vector<VkFormat>& candidates, VkImageTiling tiling, VkFormatFeatureFlags features);
		bool hasStencilComponent(VkFormat format);
	};
}
