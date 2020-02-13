#pragma once

#include "../engine/Buffer.h"
#include "../engine/Device.h"

#include <vulkan/vulkan.h>

namespace vecs {

	class Texture {
	public:
		VkImage image;
		VkFormat format;
		VkDeviceMemory deviceMemory;
		VkImageView view;
		uint32_t width, height;
		uint32_t mipLevels;
		uint32_t layerCount;
		VkDescriptorImageInfo descriptor;
		VkSampler sampler;

		static void createImageView(Device* device, VkImage image, VkFormat format, VkImageView* view,
			VkImageAspectFlags aspectFlags = VK_IMAGE_ASPECT_COLOR_BIT) {

			VkImageViewCreateInfo viewInfo = {};
			viewInfo.sType = VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO;
			viewInfo.image = image;
			viewInfo.viewType = VK_IMAGE_VIEW_TYPE_2D;
			viewInfo.format = format;
			viewInfo.subresourceRange.aspectMask = aspectFlags;
			viewInfo.subresourceRange.baseMipLevel = 0;
			viewInfo.subresourceRange.levelCount = 1;
			viewInfo.subresourceRange.baseArrayLayer = 0;
			viewInfo.subresourceRange.layerCount = 1;

			if (vkCreateImageView(device->logical, &viewInfo, nullptr, view) != VK_SUCCESS) {
				throw std::runtime_error("failed to create texture image view!");
			}
		}

		void init(Device* device, VkQueue copyQueue, const char* filename,
			VkFilter filter = VK_FILTER_NEAREST,
			VkFormat format = VK_FORMAT_R8G8B8A8_SRGB,
			VkImageUsageFlags usageFlags = VK_IMAGE_USAGE_SAMPLED_BIT,
			VkImageLayout imageLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL);

		void cleanup();

	private:
		Device* device;

		Buffer readImageData(const char* filename);
		void createImage(VkFormat format, VkImageUsageFlags usageFlags);
		void transitionImageLayout(VkCommandBuffer commandBuffer, VkFormat format, VkImageLayout oldLayout, VkImageLayout newLayout);
		void copyBufferToImage(VkCommandBuffer commandBuffer, Buffer buffer);
		void createTextureSampler(VkFilter filter);
	};
}
