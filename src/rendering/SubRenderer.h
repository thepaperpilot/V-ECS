#pragma once

#include <vulkan/vulkan.h>

#include <array>
#include <vector>
#include <unordered_set>

namespace vecs {

	// Forward Declarations
	class Device;
	class Renderer;

	struct ShaderInfo {
		std::string filepath;
		VkShaderStageFlagBits shaderStage;
		VkShaderModule shaderModule;

		ShaderInfo(std::string filepath, VkShaderStageFlagBits shaderStage) {
			this->filepath = filepath;
			this->shaderStage = shaderStage;
		}
	};

	class SubRenderer {
	public:
		Renderer* renderer;

		std::vector<VkCommandBuffer> commandBuffers;
		std::unordered_set<uint32_t> dirtyBuffers;

		void init(Device* device, Renderer* renderer);
		void markAllBuffersDirty();
		void buildCommandBuffer(uint32_t imageIndex);
		void windowRefresh(bool numImagesChanged, int imageCount);
		void cleanup();

	protected:
		Device* device;

		// Settings each subrenderer should probably set in its constructor
		std::vector<ShaderInfo> shaders;
		bool alwaysDirty = false;

		// This is made accessible to implementations so it can be used for sending push constants
		VkPipelineLayout pipelineLayout;

		// Optional method for implementations to prepare anything they need to
		// This is called after getting references to the device, renderer, and renderPass
		// but before all the values needed for the graphics pipeline are queried
		// Return true if you created the graphics pipeline yourself
		virtual void init() {};

		// This method must be overidden by an implementation that tells it how to render this
		// Prior to being run a command buffer is started, a render pass started, and the pipeline
		// and descriptor set bound. The render function should send any push constants it has
		// (if applicable) and draw the actual geometry. It should not end the buffer.
		virtual void render(VkCommandBuffer buffer) = 0;

		// These must be overidden by an implementation so we know what UBOs and samplers
		// the shader will be expecting
		// Note that pointers to objects returned in any of these functions MUST remain
		// allocated, generally by making them class members. 
		virtual std::vector<VkDescriptorSetLayoutBinding> getDescriptorLayoutBindings() { return {}; };
		virtual std::vector<VkWriteDescriptorSet> getDescriptorWrites(VkDescriptorSet descriptorSet) { return {}; };
		virtual VkVertexInputBindingDescription getBindingDescription() { return {}; };
		virtual std::vector<VkVertexInputAttributeDescription> getAttributeDescriptions() { return {}; };

		// These all can be overidden for more advanced control over the subrenderer
		// By default they'll handle things the recommended way, using the protected
		// members as configuration options as necessary. They should be set by the
		// implementation prior to registering it with the renderer
		// TODO support non-two shader stages?
		virtual std::vector<VkPipelineShaderStageCreateInfo> getShaderStages();
		virtual VkPipelineInputAssemblyStateCreateInfo getInputAssembly();
		virtual VkPipelineRasterizationStateCreateInfo getRasterizer();
		virtual VkPipelineMultisampleStateCreateInfo getMultisampling();
		virtual VkPipelineDepthStencilStateCreateInfo getDepthStencil();
		virtual std::vector< VkPipelineColorBlendAttachmentState> getColorBlendAttachments();
		virtual std::vector<VkDynamicState> getDynamicStates();
		virtual std::vector<VkPushConstantRange> getPushConstantRanges() { return {}; };
		virtual void OnWindowRefresh(bool numImagesChanged, int imageCount) {};
		virtual void cleanShaderModules();

		virtual void preRender() {};
		virtual void preCleanup() {};

	private:
		std::vector<VkDescriptorSetLayoutBinding> bindings;
		VkDescriptorSetLayout descriptorSetLayout;

		VkVertexInputBindingDescription bindingDescription;
		std::vector<VkVertexInputAttributeDescription> attributeDescriptions;
		std::vector< VkPipelineColorBlendAttachmentState> colorBlendAttachments;
		std::vector<VkDynamicState> dynamicStates;

		std::vector<VkPipelineShaderStageCreateInfo> shaderStages;
		VkPipelineVertexInputStateCreateInfo vertexInputInfo;
		VkPipelineInputAssemblyStateCreateInfo inputAssembly;
		VkPipelineViewportStateCreateInfo viewportState;
		VkPipelineRasterizationStateCreateInfo rasterizer;
		VkPipelineMultisampleStateCreateInfo multisampling;
		VkPipelineDepthStencilStateCreateInfo depthStencil;
		VkPipelineColorBlendStateCreateInfo colorBlending;
		VkPipelineDynamicStateCreateInfo dynamicState;
		std::vector<VkPushConstantRange> pushConstantRanges;

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
