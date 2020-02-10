#pragma once

#include "Vertex.h"
#include "Texture.h"
#include "../engine/Device.h"
#include "../ecs/EntityQuery.h"

#include <vulkan/vulkan.h>
#include <GLFW/glfw3.h>
#include <vector>

namespace vecs {

	// Forward Declarations
	struct QueueFamilyIndices;
	class Device;
	class World;

	class Renderer {
		friend class Engine;
	public:
		Renderer(Device* device, VkSurfaceKHR surface, GLFWwindow* window, World* world);

		VkQueue graphicsQueue;
		VkQueue presentQueue;

		VkSwapchainKHR swapChain;
		std::vector<VkImage> swapChainImages;
		// swapChainExtent contains the current width and height values of our rendered images
		VkExtent2D swapChainExtent;
		std::vector<VkCommandBuffer> commandBuffers;

		EntityQuery pushConstants;
		EntityQuery meshes;

		void recreateSwapChain();
		void createCommandBuffer(int i);

		void cleanup();

	private:
		Device* device;
		VkSurfaceKHR surface;
		GLFWwindow* window;
		World* world;

		Texture texture;
		Texture depthTexture;

		VkSurfaceFormatKHR surfaceFormat;
		VkPresentModeKHR presentMode;

		VkFormat swapChainImageFormat;

		std::vector<VkImageView> swapChainImageViews;
		std::vector<VkFramebuffer> swapChainFramebuffers;

		VkDescriptorPool descriptorPool;
		std::vector<VkDescriptorSet> descriptorSets;

		VkRenderPass renderPass;
		VkDescriptorSetLayout descriptorSetLayout;
		VkPipelineLayout pipelineLayout;
		VkPipeline graphicsPipeline;

		void cleanupSwapChain();

		void initQueueHandles();
		void createSwapChain();
		void createImageViews();
		void createRenderPass();
		void createDescriptorSetLayout();
		void createGraphicsPipeline();
		VkShaderModule createShaderModule(const std::vector<char>& code);
		void createDescriptorPool();
		void createDescriptorSets();
		void createFramebuffers();

		VkSurfaceFormatKHR chooseSwapSurfaceFormat(const std::vector<VkSurfaceFormatKHR>& availableFormats);
		VkPresentModeKHR chooseSwapPresentMode(const std::vector<VkPresentModeKHR>& availablePresentModes);
		VkExtent2D chooseSwapExtent(const VkSurfaceCapabilitiesKHR& capabilities);
	};
}
