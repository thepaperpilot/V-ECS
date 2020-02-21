#pragma once

#include "../rendering/SubRenderer.h"
#include "../rendering/Texture.h"
#include "../ecs/EntityQuery.h"

#include "../libs/imgui/imgui.h"

namespace vecs {

	// Forward Declarations
	class World;

	class GUIRenderer : public SubRenderer {
	public:
		Texture fontTexture;

		GUIRenderer();

		void init(World* world);

	protected:
		virtual void init() override;
		virtual void preRender() override;
		virtual void render(VkCommandBuffer buffer) override;
		virtual void preCleanup() override;

		virtual std::vector<VkDescriptorSetLayoutBinding> getDescriptorLayoutBindings() override;
		virtual std::vector<VkWriteDescriptorSet> getDescriptorWrites(VkDescriptorSet descriptorSet) override;
		virtual VkVertexInputBindingDescription getBindingDescription() override;
		virtual std::vector<VkVertexInputAttributeDescription> getAttributeDescriptions() override;

		virtual VkPipelineRasterizationStateCreateInfo getRasterizer() override;
		virtual VkPipelineDepthStencilStateCreateInfo getDepthStencil() override;
		virtual std::vector<VkPushConstantRange> getPushConstantRanges() override;

	private:
		World* world;

		EntityQuery guis;

		VkDescriptorImageInfo imageInfo;

		Buffer vertexBuffer;
		size_t vertexBufferSize = 0;

		Buffer indexBuffer;
		size_t indexBufferSize = 0;

		ImDrawData* draw_data;
	};
}
