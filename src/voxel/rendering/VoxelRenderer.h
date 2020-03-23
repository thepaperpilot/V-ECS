#pragma once

#include "../../rendering/SubRenderer.h"
#include "../../rendering/Texture.h"
#include "../../ecs/EntityQuery.h"
#include "../data/BlockLoader.h"
#include "Vertex.h"

#include <glm/glm.hpp>

namespace vecs {

	// Forward Declarations
	class World;
	struct CameraComponent;

	class VoxelRenderer : public SubRenderer {
	public:
		CameraComponent* camera;

		BlockLoader blockLoader;

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
