#pragma once

#include "Vertex.h"
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
		std::vector<VkCommandBuffer> commandBuffers;

		EntityQuery meshes;

		void recreateSwapChain();
		void createCommandBuffers();

		void cleanup();

	private:
		Device* device;
		VkSurfaceKHR surface;
		GLFWwindow* window;
		World* world;

		VkSurfaceFormatKHR surfaceFormat;
		VkPresentModeKHR presentMode;

		VkFormat swapChainImageFormat;
		VkExtent2D swapChainExtent;

		std::vector<VkImageView> swapChainImageViews;
		std::vector<VkFramebuffer> swapChainFramebuffers;

		VkRenderPass renderPass;
		VkPipelineLayout pipelineLayout;
		VkPipeline graphicsPipeline;

		void cleanupSwapChain();

		void initQueueHandles();
		void createSwapChain();
		void createImageViews();
		void createRenderPass();
		void createGraphicsPipeline();
		VkShaderModule createShaderModule(const std::vector<char>& code);
		void createFramebuffers();

		VkSurfaceFormatKHR chooseSwapSurfaceFormat(const std::vector<VkSurfaceFormatKHR>& availableFormats);
		VkPresentModeKHR chooseSwapPresentMode(const std::vector<VkPresentModeKHR>& availablePresentModes);
		VkExtent2D chooseSwapExtent(const VkSurfaceCapabilitiesKHR& capabilities);
	};
}
