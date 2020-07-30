#pragma once

#include <vulkan/vulkan.h>

#define SOL_DEFAULT_PASS_ON_ERROR 1
#define SOL_ALL_SAFETIES_ON 1
#include <sol\sol.hpp>

#include <array>
#include <vector>
#include <atomic>
#include <shared_mutex>

namespace vecs {

	// Forward Declarations
	class Buffer;
	class DependencyNodeLoadStatus;
	class Device;
	class LuaVal;
	class Model;
	class Renderer;
	class SecondaryCommandBuffer;
	class Texture;
	struct VertexLayout;
	class Worker;

	class SubRenderer {
	public:
		Device* device;
		Worker* worker;

		std::vector<Model*> models;
		std::vector<Texture*> textures;
		std::vector<VkSampler> immutableSamplers;

		VertexLayout* vertexLayout;
		VkPipelineLayout pipelineLayout;
		VkDescriptorSetLayout descriptorSetLayout;
		std::atomic_size_t allocatedDescriptorSets = 0;
		std::atomic_uint32_t numDescriptorPools = 0;

		// descriptor pool for use in imgui subrenderers that need to be able to load
		// many images with descriptor sets
		VkDescriptorPool imguiDescriptorPool = VK_NULL_HANDLE;

		SubRenderer(LuaVal* config, Worker* worker, Device* device, Renderer* renderer, DependencyNodeLoadStatus* status);

		SecondaryCommandBuffer* startRendering(Worker* worker);
		void finishRendering(VkCommandBuffer buffer);
		void windowRefresh(int imageCount);
		void cleanup();

		void addVertexUniform(Buffer* buffer, VkDeviceSize size);
		void addFragmentUniform(Buffer* buffer, VkDeviceSize size);

	private:
		Renderer* renderer;

		LuaVal* config;

		std::vector<VkDescriptorSetLayoutBinding> bindings;
		std::vector<VkDescriptorPool> descriptorPools;
		std::vector<std::vector<VkDescriptorSet>> descriptorSets;
		std::shared_mutex descriptorPoolsMutex;

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

		uint32_t numTextures = 0;
		uint32_t numVertexUniforms = 0;
		uint32_t numFragmentUniforms = 0;

		std::vector<VkDescriptorImageInfo> imageInfos;
		std::vector<VkDescriptorBufferInfo> vertexUniformBufferInfos;
		std::vector<VkDescriptorBufferInfo> fragmentUniformBufferInfos;

		void createDescriptorLayoutBindings();
		void createDescriptorSetLayout();
		VkDescriptorPool createDescriptorPool(size_t imageCount);
		std::vector<VkDescriptorSet> createDescriptorSets(VkDescriptorPool descriptorPool, size_t imageCount);
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

		void cleanShaderModules();
	};
}
