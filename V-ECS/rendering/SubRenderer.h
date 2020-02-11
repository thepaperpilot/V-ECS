#pragma once

#include <vulkan/vulkan.h>

#include <array>
#include <vector>
#include <unordered_set>

namespace vecs {

	// Forward Declarations
	class Device;
	class Renderer;

	class SubRenderer {
	public:
		Renderer* renderer;

		std::vector<VkCommandBuffer> commandBuffers;
		std::unordered_set<uint32_t> dirtyBuffers;

		void init(Device* device, Renderer* renderer, VkRenderPass* renderPass);
		void markAllBuffersDirty();
		void buildCommandBuffer(uint32_t imageIndex);
		void windowRefresh(bool numImagesChanged, size_t imageCount);
		void cleanup();

	protected:
		std::string vertShaderFile;
		std::string fragShaderFile;

		VkVertexInputBindingDescription (*getBindingDescription)();
		std::vector<VkVertexInputAttributeDescription> (*getAttributeDescriptions)();

		// This is made accessible to implementations so it can be used for sending push constants
		VkPipelineLayout pipelineLayout;

		// This method must be overidden by an implementation that tells it how to render this
		// Prior to being run a command buffer is started, a render pass started, and the pipeline
		// and descriptor set bound. The render function should send any push constants it has
		// (if applicable) and draw the actual geometry. It should not end the buffer.
		virtual void render(VkCommandBuffer buffer) = 0;

		// These must be overidden by an implementation so we know what UBOs and samplers
		// the shader will be expecting
		virtual std::vector<VkDescriptorSetLayoutBinding> getDescriptorLayoutBindings() = 0;
		virtual std::vector<VkWriteDescriptorSet> getDescriptorWrites(VkDescriptorSet descriptorSet) = 0;

		// These all can be overidden for more advanced control over the subrenderer
		// By default they'll handle things the recommended way, using the protected
		// members as configuration options as necessary. They should be set by the
		// implementation prior to registering it with the renderer
		// TODO support non-two shader stages?
		virtual std::array<VkPipelineShaderStageCreateInfo, 2> getShaderModules();
		virtual VkPipelineVertexInputStateCreateInfo getVertexInputInfo();
		virtual VkPipelineInputAssemblyStateCreateInfo getInputAssembly();
		virtual VkPipelineViewportStateCreateInfo getViewportState();
		virtual VkPipelineRasterizationStateCreateInfo getRasterizer();
		virtual VkPipelineMultisampleStateCreateInfo getMultisampling();
		virtual VkPipelineDepthStencilStateCreateInfo getDepthStencil();
		virtual VkPipelineColorBlendStateCreateInfo getColorBlending();
		virtual VkPipelineDynamicStateCreateInfo getDynamicState();
		virtual std::vector<VkPushConstantRange> getPushConstantRanges() { return {}; };
		virtual void OnWindowRefresh(bool numImagesChanged, size_t imageCount) {};
		virtual void cleanShaderModules();

		VkShaderModule createShaderModule(const std::vector<char>& code);

	private:
		Device* device;
		VkRenderPass* renderPass;

		std::vector<VkDescriptorSetLayoutBinding> bindings;
		VkDescriptorSetLayout descriptorSetLayout;

		std::vector<VkVertexInputAttributeDescription> attributeDescriptions;
		std::array<VkPipelineShaderStageCreateInfo, 2> shaderStages;
		VkPipelineVertexInputStateCreateInfo vertexInputInfo;
		VkPipelineInputAssemblyStateCreateInfo inputAssembly;
		VkPipelineViewportStateCreateInfo viewportState;
		VkPipelineRasterizationStateCreateInfo rasterizer;
		VkPipelineMultisampleStateCreateInfo multisampling;
		VkPipelineDepthStencilStateCreateInfo depthStencil;
		VkPipelineColorBlendStateCreateInfo colorBlending;
		VkPipelineDynamicStateCreateInfo dynamicState;
		std::vector<VkPushConstantRange> pushConstantRanges;

		VkShaderModule vertShaderModule;
		VkShaderModule fragShaderModule;

		VkPipeline graphicsPipeline;

		VkDescriptorPool descriptorPool;
		std::vector<VkDescriptorSet> descriptorSets;

		void createDescriptorSetLayout();

		void createGraphicsPipeline();

		void createDescriptorPool(size_t imageCount);
		void createDescriptorSets(size_t imageCount);
	};
}
