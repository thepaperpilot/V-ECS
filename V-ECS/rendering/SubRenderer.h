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

		void init(Device* device, Renderer* renderer);
		void markAllBuffersDirty();
		void buildCommandBuffer(uint32_t imageIndex);
		void windowRefresh(bool numImagesChanged, size_t imageCount);
		void cleanup();

	protected:
		Device* device;

		std::string vertShaderFile;
		std::string fragShaderFile;

		// This is made accessible to implementations so it can be used for sending push constants
		VkPipelineLayout pipelineLayout;

		// Optional method for implementations to prepare anything they need to
		// This is called after getting references to the device, renderer, and renderPass
		// but before all the values needed for the graphics pipeline are queried
		virtual void init() = 0;

		// This method must be overidden by an implementation that tells it how to render this
		// Prior to being run a command buffer is started, a render pass started, and the pipeline
		// and descriptor set bound. The render function should send any push constants it has
		// (if applicable) and draw the actual geometry. It should not end the buffer.
		virtual void render(VkCommandBuffer buffer) = 0;

		// These must be overidden by an implementation so we know what UBOs and samplers
		// the shader will be expecting
		// Note that pointers to objects returned in any of these functions MUST remain
		// allocated, generally by making them class members. 
		virtual std::vector<VkDescriptorSetLayoutBinding> getDescriptorLayoutBindings() = 0;
		virtual std::vector<VkWriteDescriptorSet> getDescriptorWrites(VkDescriptorSet descriptorSet) = 0;
		virtual VkVertexInputBindingDescription getBindingDescription() = 0;
		virtual std::vector<VkVertexInputAttributeDescription> getAttributeDescriptions() = 0;

		// These all can be overidden for more advanced control over the subrenderer
		// By default they'll handle things the recommended way, using the protected
		// members as configuration options as necessary. They should be set by the
		// implementation prior to registering it with the renderer
		// TODO support non-two shader stages?
		virtual std::array<VkPipelineShaderStageCreateInfo, 2> getShaderModules();
		virtual VkPipelineInputAssemblyStateCreateInfo getInputAssembly();
		virtual VkPipelineRasterizationStateCreateInfo getRasterizer();
		virtual VkPipelineMultisampleStateCreateInfo getMultisampling();
		virtual VkPipelineDepthStencilStateCreateInfo getDepthStencil();
		virtual std::vector< VkPipelineColorBlendAttachmentState> getColorBlendAttachments();
		virtual std::vector<VkDynamicState> getDynamicStates();
		virtual std::vector<VkPushConstantRange> getPushConstantRanges() { return {}; };
		virtual void OnWindowRefresh(bool numImagesChanged, size_t imageCount) {};
		virtual void cleanShaderModules();

		VkShaderModule createShaderModule(const std::vector<char>& code);

		virtual void preCleanup() = 0;

	private:
		std::vector<VkDescriptorSetLayoutBinding> bindings;
		VkDescriptorSetLayout descriptorSetLayout;

		VkVertexInputBindingDescription bindingDescription;
		std::vector<VkVertexInputAttributeDescription> attributeDescriptions;
		std::vector< VkPipelineColorBlendAttachmentState> colorBlendAttachments;
		std::vector<VkDynamicState> dynamicStates;

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

		std::vector<VkCommandBufferInheritanceInfo> inheritanceInfo;

		void createDescriptorSetLayout();

		void createGraphicsPipeline();

		VkPipelineVertexInputStateCreateInfo getVertexInputInfo();
		VkPipelineColorBlendStateCreateInfo getColorBlending();
		VkPipelineDynamicStateCreateInfo getDynamicState();

		void createDescriptorPool(size_t imageCount);
		void createDescriptorSets(size_t imageCount);

		void createInheritanceInfo(size_t imageCount);
	};
}
