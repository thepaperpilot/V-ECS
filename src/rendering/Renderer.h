#pragma once

#include "DepthTexture.h"

#include <vulkan/vulkan.h>
#include <GLFW/glfw3.h>

#include <vector>
#include <set>
#include <mutex>

const uint32_t maxFramesInFlight = 2;
const int START_RENDERING_PRIORITY = 2000;

namespace vecs {

	// Forward Declarations
	class Engine;
	class Device;
	struct RefreshWindowEvent;
	class SecondaryCommandBuffer;
	class Worker;

	class Renderer {
	friend class SubRenderer;
	friend class SecondaryCommandBuffer;
	friend class Worker;
	public:
		GLFWwindow* window;

		uint32_t imageCount = 0;
		VkExtent2D swapChainExtent;

		Renderer(Engine* engine) {
			this->engine = engine;
		}

		void init(Device* device, VkSurfaceKHR surface, GLFWwindow* window);
		void acquireImage();
		void presentImage();
		bool refreshWindow(RefreshWindowEvent* ignored = nullptr);

		void cleanup();

	private:
		Engine* engine;
		Device* device;
		VkSurfaceKHR surface;

		VkQueue graphicsQueue;
		VkQueue presentQueue;

		VkSurfaceFormatKHR surfaceFormat;
		VkFormat swapChainImageFormat;

		DepthTexture depthTexture;
		VkRenderPass renderPass;

		VkSwapchainKHR swapChain;
		VkCommandPool commandPool;
		std::vector<VkImage> swapChainImages;
		std::vector<VkImageView> swapChainImageViews;
		std::vector<VkFramebuffer> swapChainFramebuffers;
		std::vector<VkCommandBuffer> commandBuffers;
		std::vector<VkCommandBuffer> secondaryBuffers;
		std::mutex secondaryBufferMutex;

		size_t currentFrame = 0;
		uint32_t imageIndex;
		std::vector<VkSemaphore> imageAvailableSemaphores;
		std::vector<VkSemaphore> renderFinishedSemaphores;
		std::vector<VkFence> inFlightFences;
		std::vector<VkFence> imagesInFlight;

		void createRenderPass();

		bool createSwapChain(VkSwapchainKHR* oldSwapChain = nullptr);
		void createFramebuffers();
		void createImageViews();

		void buildCommandBuffer();

		VkSurfaceFormatKHR chooseSwapSurfaceFormat(const std::vector<VkSurfaceFormatKHR>& availableFormats);
		VkPresentModeKHR chooseSwapPresentMode(const std::vector<VkPresentModeKHR>& availablePresentModes);
		VkExtent2D chooseSwapExtent(const VkSurfaceCapabilitiesKHR& capabilities);
	};
}
