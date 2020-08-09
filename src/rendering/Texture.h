#pragma once

#include "../engine/Buffer.h"
#include "../engine/Debugger.h"
#include "../engine/Device.h"

#include <vulkan/vulkan.h>

#include <imgui.h>

namespace vecs {

	// Forward Declarations
	class Device;
	class SubRenderer;
	class Worker;

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
			VkImageAspectFlags aspectFlags = VK_IMAGE_ASPECT_COLOR_BIT);

		Texture(SubRenderer* subrenderer, Worker* worker, const char* filename,
			bool addToSubrenderer = true,
			VkFilter filter = VK_FILTER_NEAREST,
			VkFormat format = VK_FORMAT_R8G8B8A8_SRGB,
			VkImageUsageFlags usageFlags = VK_IMAGE_USAGE_SAMPLED_BIT,
			VkImageLayout imageLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL);

		Texture(SubRenderer* subrenderer, Worker* worker, unsigned char* pixels,
			int width, int height,
			bool addToSubrenderer = true,
			VkFilter filter = VK_FILTER_NEAREST,
			VkFormat format = VK_FORMAT_R8G8B8A8_UNORM,
			VkImageUsageFlags usageFlags = VK_IMAGE_USAGE_SAMPLED_BIT,
			VkImageLayout imageLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL);

		void cleanup();

	private:
		Device* device;

		void init(Buffer buffer, Worker* worker, VkFilter filter,
			VkImageUsageFlags usageFlags, VkImageLayout imageLayout);

		Buffer readImageData(const char* filename);
		Buffer readPixels(unsigned char* pixels, int width, int height);
		void createImage(VkFormat format, VkImageUsageFlags usageFlags);
		void transitionImageLayout(VkCommandBuffer commandBuffer, VkFormat format, VkImageLayout oldLayout, VkImageLayout newLayout);
		void copyBufferToImage(VkCommandBuffer commandBuffer, Buffer buffer);
		void createTextureSampler(VkFilter filter);
	};
}
