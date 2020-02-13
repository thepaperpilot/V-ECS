#include "VoxelRenderer.h"
#include "MeshComponent.h"
#include "../../ecs/World.h"

#include <glm/glm.hpp>

#define GLM_ENABLE_EXPERIMENTAL
#include <glm/gtx/string_cast.hpp>

using namespace vecs;

VoxelRenderer::VoxelRenderer() {
	// Configure shaders
	vertShaderFile = "shaders/shader-vert.spv";
	fragShaderFile = "shaders/shader-frag.spv";
}

void VoxelRenderer::init(World* world) {
	this->world = world;
	// Add entity query so we know what to render
	meshes.filter.with(typeid(MeshComponent));
	world->addQuery(&meshes);
}

void VoxelRenderer::init() {
	// Create our texture and 
	texture.init(device, renderer->graphicsQueue, "textures/rgb.png");

	// Create the imageinfo we'll need for our descriptor writes
	imageInfo.imageLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
	imageInfo.imageView = texture.view;
	imageInfo.sampler = texture.sampler;
}

void VoxelRenderer::render(VkCommandBuffer commandBuffer) {
	uint32_t size = sizeof(glm::mat4);
	vkCmdPushConstants(commandBuffer, pipelineLayout, VK_SHADER_STAGE_VERTEX_BIT, 0, size, model);
	vkCmdPushConstants(commandBuffer, pipelineLayout, VK_SHADER_STAGE_VERTEX_BIT, size, size, view);
	vkCmdPushConstants(commandBuffer, pipelineLayout, VK_SHADER_STAGE_VERTEX_BIT, size * 2, size, projection);

	// TODO I don't think this is how to render multiple "meshes"
	for (auto const entity : meshes.entities) {
		MeshComponent mesh = *world->getComponent<MeshComponent>(entity);
		if (mesh.vertices.empty()) continue;

		// Bind our vertex and index buffers
		VkBuffer vertexBuffers[] = { mesh.vertexBuffer };
		VkDeviceSize offsets[] = { 0 };
		vkCmdBindVertexBuffers(commandBuffer, 0, 1, vertexBuffers, offsets);
		vkCmdBindIndexBuffer(commandBuffer, mesh.indexBuffer, 0, VK_INDEX_TYPE_UINT16);

		// Draw our vertices
		vkCmdDrawIndexed(commandBuffer, static_cast<uint32_t>(mesh.indices.size()), 1, 0, 0, 0);
	}
}

void VoxelRenderer::preCleanup() {
	texture.cleanup();
}

std::vector<VkDescriptorSetLayoutBinding> VoxelRenderer::getDescriptorLayoutBindings() {
	VkDescriptorSetLayoutBinding samplerLayoutBinding = {};
	samplerLayoutBinding.binding = 1;
	samplerLayoutBinding.descriptorCount = 1;
	samplerLayoutBinding.descriptorType = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
	samplerLayoutBinding.pImmutableSamplers = nullptr;
	samplerLayoutBinding.stageFlags = VK_SHADER_STAGE_FRAGMENT_BIT;
	return { samplerLayoutBinding };
}

std::vector<VkWriteDescriptorSet> VoxelRenderer::getDescriptorWrites(VkDescriptorSet descriptorSet) {
	VkWriteDescriptorSet descriptorWrite = {};
	descriptorWrite.sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
	descriptorWrite.dstSet = descriptorSet;
	descriptorWrite.dstBinding = 1;
	descriptorWrite.dstArrayElement = 0;
	descriptorWrite.descriptorType = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
	descriptorWrite.descriptorCount = 1;
	descriptorWrite.pImageInfo = &imageInfo;

	return { descriptorWrite };
}

std::vector<VkPushConstantRange> VoxelRenderer::getPushConstantRanges() {
	VkPushConstantRange pushConstantRange = {};
	pushConstantRange.stageFlags = VK_SHADER_STAGE_VERTEX_BIT;
	pushConstantRange.offset = 0;
	pushConstantRange.size = 3 * sizeof(glm::mat4);

	return { pushConstantRange };
}
