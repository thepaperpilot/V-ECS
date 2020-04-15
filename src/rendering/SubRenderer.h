#pragma once

#include <vulkan/vulkan.h>

#define SOL_ALL_SAFETIES_ON 1
#include <sol\sol.hpp>

#include <array>
#include <vector>

#include "VertexLayout.h"
#include "Model.h"
#include "Texture.h"

namespace vecs {

	// Forward Declarations
	class Device;
	class Renderer;

	class SubRenderer {
	public:
		Device* device;
		Renderer* renderer;

		std::vector<Model*> models;
		std::vector<Texture*> textures;
		std::vector<VkSampler> immutableSamplers;

		VertexLayout* vertexLayout;
		VkCommandBuffer activeCommandBuffer = VK_NULL_HANDLE;
		VkPipelineLayout pipelineLayout;

		SubRenderer(Device* device, Renderer* renderer, sol::table worldConfig, sol::table config);

		void buildCommandBuffer(sol::table worldConfig);
		void windowRefresh(bool numImagesChanged, int imageCount);
		void cleanup();

	private:
		sol::table config;

		std::vector<VkCommandBuffer> commandBuffers;

		std::vector<VkDescriptorSetLayoutBinding> bindings;
		VkDescriptorSetLayout descriptorSetLayout;
		VkDescriptorPool descriptorPool = VK_NULL_HANDLE;
		std::vector<VkDescriptorSet> descriptorSets;

		std::vector<VkPipelineShaderStageCreateInfo> shaderStages;
		std::vector<VkShaderModule> shaderModules;
		VkPipelineVertexInputStateCreateInfo vertexInput;
		VkPipelineInputAssemblyStateCreateInfo inputAssembly;
		VkPipelineRasterizationStateCreateInfo rasterizer;
		VkPipelineMultisampleStateCreateInfo multisampling;
		VkPipelineDepthStencilStateCreateInfo depthStencil;
		VkPipelineColorBlendAttachmentState colorBlendAttachment;
		VkPipelineColorBlendStateCreateInfo colorBlending;
		std::vector<VkDynamicState> dynamicStates = { VK_DYNAMIC_STATE_VIEWPORT, VK_DYNAMIC_STATE_SCISSOR };
		VkPipelineDynamicStateCreateInfo dynamicState;
		std::vector<VkPushConstantRange> pushConstantRanges;

		VkPipeline graphicsPipeline;
		std::vector<VkCommandBufferInheritanceInfo> inheritanceInfo;

		uint32_t numTextures = 0;
		uint32_t numVertexUniforms = 0;
		uint32_t numFragmentUniforms = 0;

		std::vector<VkDescriptorImageInfo> imageInfos;
		std::vector<VkDescriptorBufferInfo> vertexUniformBufferInfos;
		std::vector<VkDescriptorBufferInfo> fragmentUniformBufferInfos;

		void createDescriptorLayoutBindings();
		void createDescriptorSetLayout();
		void createDescriptorPool(size_t imageCount);
		void createDescriptorSets(size_t imageCount);
		std::vector<VkWriteDescriptorSet> getDescriptorWrites(VkDescriptorSet descriptorSet);

		void createShaderStages();
		void createVertexInput();
		void createInputAssembly();
		void createRasterizer();
		void createMultisampling();
		void createDepthStencil();
		void createColorBlendAttachments();
		void createDynamicStates();
		void createPushConstantRanges();

		void createGraphicsPipeline();
		void createInheritanceInfo(size_t imageCount);

		void cleanShaderModules();
	};
}
