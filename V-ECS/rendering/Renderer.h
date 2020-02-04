#pragma once

#include "Vertex.h"
#include <vulkan/vulkan.h>
#include <vector>

namespace vecs {

	// Forward Declarations
	struct QueueFamilyIndices;
	class Engine;

	struct SwapChainSupportDetails {
		VkSurfaceCapabilitiesKHR capabilities;
		std::vector<VkSurfaceFormatKHR> formats;
		std::vector<VkPresentModeKHR> presentModes;
	};

	class Renderer {
	public:
		Engine* engine;

		VkCommandPool commandPool;

		VkQueue graphicsQueue;
		VkQueue presentQueue;

		VkSwapchainKHR swapChain;
		std::vector<VkImage> swapChainImages;
		std::vector<VkCommandBuffer> commandBuffers;

		VkBuffer vertexBuffer;
		std::vector<Vertex> vertices;

		void init(QueueFamilyIndices indices, SwapChainSupportDetails swapChainSupport);
		void recreateSwapChain();
		void createCommandBuffers();

		SwapChainSupportDetails querySwapChainSupport(VkPhysicalDevice physicalDevice);

		void cleanup();

	private:
		VkFormat swapChainImageFormat;
		VkExtent2D swapChainExtent;

		std::vector<VkImageView> swapChainImageViews;
		std::vector<VkFramebuffer> swapChainFramebuffers;

		VkRenderPass renderPass;
		VkPipelineLayout pipelineLayout;
		VkPipeline graphicsPipeline;

		void cleanupSwapChain();

		void initQueueHandles(QueueFamilyIndices indices);
		void createSwapChain(QueueFamilyIndices indices, SwapChainSupportDetails swapChainSupport);
		void createImageViews();
		void createRenderPass();
		void createGraphicsPipeline();
		VkShaderModule createShaderModule(const std::vector<char>& code);
		void createFramebuffers();
		void createCommandPool(QueueFamilyIndices indices);

		VkSurfaceFormatKHR chooseSwapSurfaceFormat(const std::vector<VkSurfaceFormatKHR>& availableFormats);
		VkPresentModeKHR chooseSwapPresentMode(const std::vector<VkPresentModeKHR>& availablePresentModes);
		VkExtent2D chooseSwapExtent(const VkSurfaceCapabilitiesKHR& capabilities);
	};
}
