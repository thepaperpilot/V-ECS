#pragma once

#include "../engine/Buffer.h"
#include "../engine/Device.h"
#include "../engine/Debugger.h"

#include <vulkan/vulkan.h>

#include <imgui.h>

namespace vecs {

	// Forward Declarations
	class SubRenderer;

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
		VkImageLayout imageLayout;

		ImTextureID imguiTexId;

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

			VK_CHECK_RESULT(vkCreateImageView(device->logical, &viewInfo, nullptr, view));
		}

		Texture(SubRenderer* subrenderer, const char* filename,
			VkFilter filter = VK_FILTER_NEAREST,
			VkFormat format = VK_FORMAT_R8G8B8A8_SRGB,
			VkImageUsageFlags usageFlags = VK_IMAGE_USAGE_SAMPLED_BIT,
			VkImageLayout imageLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL);

		Texture(SubRenderer* subrenderer, unsigned char* pixels,
			int width, int height,
			VkFilter filter = VK_FILTER_NEAREST,
			VkFormat format = VK_FORMAT_R8G8B8A8_UNORM,
			VkImageUsageFlags usageFlags = VK_IMAGE_USAGE_SAMPLED_BIT,
			VkImageLayout imageLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL);

		void cleanup();

	private:
		Device* device;

		void init(Buffer buffer, VkQueue copyQueue, VkCommandPool commandPool, VkFilter filter,
			VkImageUsageFlags usageFlags, VkImageLayout imageLayout);

		Buffer readImageData(const char* filename);
		Buffer readPixels(unsigned char* pixels, int width, int height);
		void createImage(VkFormat format, VkImageUsageFlags usageFlags);
		void transitionImageLayout(VkCommandBuffer commandBuffer, VkFormat format, VkImageLayout oldLayout, VkImageLayout newLayout);
		void copyBufferToImage(VkCommandBuffer commandBuffer, Buffer buffer);
		void createTextureSampler(VkFilter filter);
	};
}
