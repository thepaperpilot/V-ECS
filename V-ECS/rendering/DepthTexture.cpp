#include "DepthTexture.h"
#include "Texture.h"

using namespace vecs;

void DepthTexture::init(Device* device, VkQueue copyQueue, VkExtent2D extent) {
	this->device = device;
	width = extent.width;
	height = extent.height;

	// Create our VkImage and VkDeviceMemory objects to store this file in
	format = findSupportedFormat(
		{ VK_FORMAT_D32_SFLOAT, VK_FORMAT_D32_SFLOAT_S8_UINT, VK_FORMAT_D24_UNORM_S8_UINT },
		VK_IMAGE_TILING_OPTIMAL,
		VK_FORMAT_FEATURE_DEPTH_STENCIL_ATTACHMENT_BIT
	);
	createImage(format, VK_IMAGE_USAGE_DEPTH_STENCIL_ATTACHMENT_BIT);
	// Create our image view
	Texture::createImageView(device, image, format, &view, VK_IMAGE_ASPECT_DEPTH_BIT);

	// Depth textures don't need samplers nor a specified imageLayout
}

void DepthTexture::cleanup() {
	vkDestroyImageView(*device, view, nullptr);
	vkDestroyImage(*device, image, nullptr);
	vkFreeMemory(*device, deviceMemory, nullptr);
}

void DepthTexture::createImage(VkFormat format, VkImageUsageFlags usageFlags) {
	VkImageCreateInfo imageInfo = {};
	imageInfo.sType = VK_STRUCTURE_TYPE_IMAGE_CREATE_INFO;
	imageInfo.imageType = VK_IMAGE_TYPE_2D;
	imageInfo.extent.width = width;
	imageInfo.extent.height = height;
	imageInfo.extent.depth = 1;
	imageInfo.mipLevels = 1;
	imageInfo.arrayLayers = 1;
	imageInfo.format = format;
	imageInfo.tiling = VK_IMAGE_TILING_OPTIMAL;
	imageInfo.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;
	imageInfo.usage = usageFlags | VK_IMAGE_USAGE_TRANSFER_DST_BIT;
	imageInfo.sharingMode = VK_SHARING_MODE_EXCLUSIVE;
	imageInfo.samples = VK_SAMPLE_COUNT_1_BIT;

	if (vkCreateImage(*device, &imageInfo, nullptr, &image) != VK_SUCCESS) {
		throw std::runtime_error("failed to create image!");
	}

	VkMemoryRequirements memRequirements;
	vkGetImageMemoryRequirements(*device, image, &memRequirements);

	VkMemoryAllocateInfo allocInfo = {};
	allocInfo.sType = VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO;
	allocInfo.allocationSize = memRequirements.size;
	allocInfo.memoryTypeIndex =
		device->findMemoryType(memRequirements.memoryTypeBits, VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT);

	if (vkAllocateMemory(*device, &allocInfo, nullptr, &deviceMemory) != VK_SUCCESS) {
		throw std::runtime_error("failed to allocate image memory!");
	}

	vkBindImageMemory(*device, image, deviceMemory, 0);
}

VkFormat DepthTexture::findSupportedFormat(const std::vector<VkFormat>& candidates,
	VkImageTiling tiling, VkFormatFeatureFlags features) {

	for (VkFormat format : candidates) {
		VkFormatProperties props;
		vkGetPhysicalDeviceFormatProperties(device->physical, format, &props);

		if (tiling == VK_IMAGE_TILING_LINEAR &&
			(props.linearTilingFeatures & features) == features) {
			return format;
		}
		else if (tiling == VK_IMAGE_TILING_OPTIMAL &&
			(props.optimalTilingFeatures & features) == features) {
			return format;
		}
	}

	throw std::runtime_error("failed to find supported format!");
}

bool DepthTexture::hasStencilComponent(VkFormat format) {
	return format == VK_FORMAT_D32_SFLOAT_S8_UINT || format == VK_FORMAT_D24_UNORM_S8_UINT;
}
