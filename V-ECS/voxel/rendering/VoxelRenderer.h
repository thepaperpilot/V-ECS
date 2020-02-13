#pragma once

#include "../../rendering/SubRenderer.h"
#include "../../rendering/Texture.h"
#include "../../ecs/EntityQuery.h"
#include "Vertex.h"

#include <glm/glm.hpp>

namespace vecs {

	// Forward Declarations
	class World;

	class VoxelRenderer : public SubRenderer {
	public:
		Texture texture;

		glm::mat4* model;
		glm::mat4* view;
		glm::mat4* projection;

		VoxelRenderer();

		void init(World* world);

	protected:
		virtual void init() override;
		virtual void render(VkCommandBuffer commandBuffer) override;
		virtual void preCleanup() override;

		virtual std::vector<VkDescriptorSetLayoutBinding> getDescriptorLayoutBindings() override;
		virtual std::vector<VkWriteDescriptorSet> getDescriptorWrites(VkDescriptorSet descriptorSet) override;
		virtual VkVertexInputBindingDescription getBindingDescription() override { return Vertex::getBindingDescription(); };
		virtual std::vector<VkVertexInputAttributeDescription> getAttributeDescriptions() override { return Vertex::getAttributeDescriptions(); };
		
		virtual std::vector<VkPushConstantRange> getPushConstantRanges() override;

	private:
		World* world;

		EntityQuery meshes;

		VkDescriptorImageInfo imageInfo;
	};
}
