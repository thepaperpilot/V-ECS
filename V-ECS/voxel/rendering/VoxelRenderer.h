#pragma once

#include "../../rendering/SubRenderer.h"
#include "../../rendering/Texture.h"
#include "../../ecs/EntityQuery.h"

namespace vecs {

	// Forward Declarations
	class World;

	class VoxelRenderer : public SubRenderer {
	public:
		Texture texture;

		VoxelRenderer();

		void init(World* world);

	protected:
		virtual void render(VkCommandBuffer commandBuffer) override;

		virtual std::vector<VkDescriptorSetLayoutBinding> getDescriptorLayoutBindings() override;
		virtual std::vector<VkWriteDescriptorSet> getDescriptorWrites(VkDescriptorSet descriptorSet) override;
		
		virtual std::vector<VkPushConstantRange> getPushConstantRanges() override;

	private:
		World* world;

		EntityQuery pushConstants;
		EntityQuery meshes;
	};
}
