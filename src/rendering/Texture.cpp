#include "Texture.h"

#include "Renderer.h"
#include "SubRenderer.h"

#define STB_IMAGE_IMPLEMENTATION
#include <stb/stb_image.h>

using namespace vecs;

Texture::Texture(SubRenderer* subrenderer, const char* filename,
	VkFilter filter, VkFormat format, VkImageUsageFlags usageFlags, VkImageLayout imageLayout) {

	this->device = subrenderer->device;
	this->format = format;

	// Read our texture data from file into a buffer
	Buffer stagingBuffer = readImageData(filename);

	init(&stagingBuffer, subrenderer->renderer->graphicsQueue, filter, usageFlags, imageLayout);

	subrenderer->textures.emplace_back(this);
}

Texture::Texture(SubRenderer* subrenderer, unsigned char* pixels, int width, int height,
	VkFilter filter, VkFormat format, VkImageUsageFlags usageFlags, VkImageLayout imageLayout) {

	this->device = subrenderer->device;
	this->format = format;

	// Read our texture data from file into a buffer
	Buffer stagingBuffer = readPixels(pixels, width, height);

	init(&stagingBuffer, subrenderer->renderer->graphicsQueue, filter, usageFlags, imageLayout);

	subrenderer->textures.emplace_back(this);
}

void Texture::cleanup() {
	vkDestroySampler(*device, sampler, nullptr);
	vkDestroyImageView(*device, view, nullptr);

	vkDestroyImage(*device, image, nullptr);
	vkFreeMemory(*device, deviceMemory, nullptr);
}

void Texture::init(Buffer* buffer, VkQueue copyQueue, VkFilter filter, VkImageUsageFlags usageFlags, VkImageLayout imageLayout) {
	this->imageLayout = imageLayout;

	// Creating our image is going to require several commands, so we'll create a buffer for them
	VkCommandBuffer commandBuffer = device->createCommandBuffer(VK_COMMAND_BUFFER_LEVEL_PRIMARY);

	// Create our VkImage and VkDeviceMemory objects to store this file in
	createImage(format, usageFlags);
	// Transition our image layout
	transitionImageLayout(commandBuffer, format, VK_IMAGE_LAYOUT_UNDEFINED, VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL);
	// Copy the image data from our staging buffer to our image
	copyBufferToImage(commandBuffer, buffer);
	// Transition our image layout again to prepare it for shader access
	transitionImageLayout(commandBuffer, format, VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL, imageLayout);

	// Submit our command buffer with all its commands in it
	device->submitCommandBuffer(commandBuffer, copyQueue);

	// Destroy our staging buffer now that we're done with it
	buffer->cleanup();

	// Create our image view
	Texture::createImageView(device, image, format, &view);

	// Create our 2D Sampler
	createTextureSampler(filter);

	// Fill out our descriptor so we can use it in descriptor sets
	descriptor.imageLayout = imageLayout;
	descriptor.imageView = view;
	descriptor.sampler = sampler;
}

Buffer Texture::readImageData(const char* filename) {
	// Read our image from file
	int texWidth, texHeight, texChannels;
	stbi_uc* pixels = stbi_load(filename, &texWidth, &texHeight, &texChannels, STBI_rgb_alpha);

	if (!pixels) {
		throw std::runtime_error("failed to load texture image!");
	}

	Buffer stagingBuffer = readPixels(pixels, texWidth, texHeight);

	// Free the image data now that we've copied it the buffer
	stbi_image_free(pixels);

	return stagingBuffer;
}

Buffer Texture::readPixels(unsigned char* pixels, int width, int height) {
	this->width = width;
	this->height = height;
	VkDeviceSize imageSize = (VkDeviceSize)width * height * 4;

	// Create a staging buffer to transfer the image to the GPU
	Buffer stagingBuffer = device->createBuffer(imageSize,
		VK_BUFFER_USAGE_TRANSFER_SRC_BIT,
		VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT);

	// Copy our image data to the staging buffer
	stagingBuffer.copyTo(pixels, imageSize);

	return stagingBuffer;
}

void Texture::createImage(VkFormat format, VkImageUsageFlags usageFlags) {
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

	VK_CHECK_RESULT(vkCreateImage(*device, &imageInfo, nullptr, &image));

	VkMemoryRequirements memRequirements;
	vkGetImageMemoryRequirements(*device, image, &memRequirements);

	VkMemoryAllocateInfo allocInfo = {};
	allocInfo.sType = VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO;
	allocInfo.allocationSize = memRequirements.size;
	allocInfo.memoryTypeIndex =
		device->findMemoryType(memRequirements.memoryTypeBits, VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT);

	VK_CHECK_RESULT(vkAllocateMemory(*device, &allocInfo, nullptr, &deviceMemory));

	vkBindImageMemory(*device, image, deviceMemory, 0);
}

void Texture::transitionImageLayout(VkCommandBuffer commandBuffer, VkFormat format,
	VkImageLayout oldLayout, VkImageLayout newLayout) {

	VkImageMemoryBarrier barrier = {};
	barrier.sType = VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER;
	barrier.oldLayout = oldLayout;
	barrier.newLayout = newLayout;
	barrier.srcQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
	barrier.dstQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
	barrier.image = image;
	barrier.subresourceRange.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
	barrier.subresourceRange.baseMipLevel = 0;
	barrier.subresourceRange.levelCount = 1;
	barrier.subresourceRange.baseArrayLayer = 0;
	barrier.subresourceRange.layerCount = 1;

	VkPipelineStageFlags sourceStage;
	VkPipelineStageFlags destinationStage;

	if (oldLayout == VK_IMAGE_LAYOUT_UNDEFINED &&
		newLayout == VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL) {

		barrier.srcAccessMask = 0;
		barrier.dstAccessMask = VK_ACCESS_TRANSFER_WRITE_BIT;

		sourceStage = VK_PIPELINE_STAGE_TOP_OF_PIPE_BIT;
		destinationStage = VK_PIPELINE_STAGE_TRANSFER_BIT;
	} else if (oldLayout == VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL &&
		newLayout == VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL) {

		barrier.srcAccessMask = VK_ACCESS_TRANSFER_WRITE_BIT;
		barrier.dstAccessMask = VK_ACCESS_SHADER_READ_BIT;

		sourceStage = VK_PIPELINE_STAGE_TRANSFER_BIT;
		destinationStage = VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT;
	} else {
		throw std::invalid_argument("unsupported layout transition!");
	}

	// Add our pipeline command to our buffer
	vkCmdPipelineBarrier(
		commandBuffer,
		sourceStage, destinationStage,
		0,
		0, nullptr,
		0, nullptr,
		1, &barrier
	);
}

void Texture::copyBufferToImage(VkCommandBuffer commandBuffer, Buffer* buffer) {
	VkBufferImageCopy region = {};
	region.bufferOffset = 0;
	region.bufferRowLength = 0;
	region.bufferImageHeight = 0;

	region.imageSubresource.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
	region.imageSubresource.mipLevel = 0;
	region.imageSubresource.baseArrayLayer = 0;
	region.imageSubresource.layerCount = 1;

	region.imageOffset = { 0, 0, 0 };
	region.imageExtent = {
		width,
		height,
		1
	};

	// Add our copy command to our buffer
	vkCmdCopyBufferToImage(
		commandBuffer,
		buffer->buffer,
		image,
		VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL,
		1,
		&region
	);
}

void Texture::createTextureSampler(VkFilter filter) {
	VkSamplerCreateInfo samplerInfo = {};
	samplerInfo.sType = VK_STRUCTURE_TYPE_SAMPLER_CREATE_INFO;
	samplerInfo.magFilter = filter;
	samplerInfo.minFilter = filter;
	samplerInfo.addressModeU = VK_SAMPLER_ADDRESS_MODE_REPEAT;
	samplerInfo.addressModeV = VK_SAMPLER_ADDRESS_MODE_REPEAT;
	samplerInfo.addressModeW = VK_SAMPLER_ADDRESS_MODE_REPEAT;
	samplerInfo.anisotropyEnable = VK_TRUE;
	samplerInfo.maxAnisotropy = 16;
	samplerInfo.borderColor = VK_BORDER_COLOR_INT_OPAQUE_BLACK;
	samplerInfo.unnormalizedCoordinates = VK_FALSE;
	samplerInfo.compareEnable = VK_FALSE;
	samplerInfo.compareOp = VK_COMPARE_OP_ALWAYS;
	samplerInfo.mipmapMode = VK_SAMPLER_MIPMAP_MODE_LINEAR;
	samplerInfo.mipLodBias = 0.0f;
	samplerInfo.minLod = 0.0f;
	samplerInfo.maxLod = 0.0f;

	VK_CHECK_RESULT(vkCreateSampler(*device, &samplerInfo, nullptr, &sampler));
}
