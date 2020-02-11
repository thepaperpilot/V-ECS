#include "VoxelRenderer.h"
#include "Vertex.h"
#include "PushConstantComponent.h"
#include "MeshComponent.h"
#include "../../ecs/World.h"

#include <glm/glm.hpp>

using namespace vecs;

VoxelRenderer::VoxelRenderer() {
	// Configure shaders
	vertShaderFile = "shaders/shader-vert.spv";
	fragShaderFile = "shaders/shader-frag.spv";

	getBindingDescription = Vertex::getBindingDescription;
	getAttributeDescriptions = Vertex::getAttributeDescriptions;
}

void VoxelRenderer::init(World* world) {
	this->world = world;
	// Add entity queries so we know what to render
	pushConstants.filter.with(typeid(PushConstantComponent));
	meshes.filter.with(typeid(MeshComponent));
	world->addQuery(&pushConstants);
	world->addQuery(&meshes);
}

void VoxelRenderer::render(VkCommandBuffer commandBuffer) {
	// TODO Push constants probably shouldn't be components. They should be explicitly set by the sub-renderer
	uint32_t offset = 0;
	for (auto const entity : pushConstants.entities) {
		PushConstantComponent* pushConstant = world->getComponent<PushConstantComponent>(entity);
		for (auto constant : pushConstant->constants) {
			vkCmdPushConstants(commandBuffer, pipelineLayout, constant.stageFlags, offset, constant.size, constant.data);
			offset += constant.size;
		}
	}

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
	VkDescriptorImageInfo imageInfo = {};
	imageInfo.imageLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
	imageInfo.imageView = texture.view;
	imageInfo.sampler = texture.sampler;

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
