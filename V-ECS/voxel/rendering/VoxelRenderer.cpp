#include "VoxelRenderer.h"
#include "MeshComponent.h"
#include "../../libs/FrustumCull.h"
#include "../../ecs/World.h"

#include <glm/glm.hpp>

#define GLM_ENABLE_EXPERIMENTAL
#include <glm/gtx/string_cast.hpp>

using namespace vecs;

VoxelRenderer::VoxelRenderer() {
	// Configure shaders
	vertShaderFile = "resources/shaders/shader-vert.spv";
	fragShaderFile = "resources/shaders/shader-frag.spv";
}

void VoxelRenderer::init(World* world) {
	this->world = world;
	blockLoader.init(world);
	// Add entity query so we know what to render
	meshes.filter.with(typeid(MeshComponent));
	world->addQuery(&meshes);
}

void VoxelRenderer::init() {
	// Load our blocks into one big texture
	imageInfo = blockLoader.loadBlocks(device, renderer->graphicsQueue);
}

void VoxelRenderer::render(VkCommandBuffer commandBuffer) {
	// Calculate our model-projection-view matrix
	// Remember that matrix multiplication order is reversed
	MVP = *projection * *view * *model;

	// Send our MVP matrix to the GPU as a push constant
	vkCmdPushConstants(commandBuffer, pipelineLayout, VK_SHADER_STAGE_VERTEX_BIT, 0, sizeof(glm::mat4), &MVP);
	// Calculate our frustum for culling purposes
	Frustum frustum(MVP);

	for (auto archetype : meshes.matchingArchetypes) {
		for (auto component : *archetype->getComponentList(typeid(MeshComponent))) {
			MeshComponent* mesh = static_cast<MeshComponent*>(component);

			// Exit early if our mesh is empty
			if (mesh->vertices.empty()) continue;

			// Skip this mesh if its outside our frustum
			if (!frustum.IsBoxVisible(mesh->minBounds, mesh->maxBounds)) continue;

			// Bind our vertex and index buffers
			VkBuffer vertexBuffers[] = { mesh->vertexBuffer };
			VkDeviceSize offsets[] = { 0 };
			vkCmdBindVertexBuffers(commandBuffer, 0, 1, vertexBuffers, offsets);
			vkCmdBindIndexBuffer(commandBuffer, mesh->indexBuffer, 0, VK_INDEX_TYPE_UINT16);

			// Draw our vertices
			vkCmdDrawIndexed(commandBuffer, static_cast<uint32_t>(mesh->indices.size()), 1, 0, 0, 0);
		}
	}
}

void VoxelRenderer::preCleanup() {
	blockLoader.cleanup();
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
	pushConstantRange.size = sizeof(glm::mat4);

	return { pushConstantRange };
}
