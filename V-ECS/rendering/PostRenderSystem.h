#pragma once

#include "../ecs/System.h"
#include "Vertex.h"
#include <vulkan/vulkan.h>
#include <GLFW/glfw3.h>
#include <vector>

namespace vecs {

	// Forward declarations
	class Engine;
	struct RefreshWindowEvent;
	struct WindowResizeEvent;
	struct AddVerticesEvent;
	struct RemoveVerticesEvent;

	class PostRenderSystem : public System {
	public:
		PostRenderSystem(Engine* engine, uint32_t maxFramesInFlight, size_t initialVertexBufferSize);

		bool framebufferResized = false;

		void update() override;
		void cleanup();

	private:
		// Pure ECS isn't completely necessary
		// I'm fine with having some information in a system,
		// if it isn't specific to any entity like these
		// No need for "singleton" components
		Engine* engine;
		uint32_t maxFramesInFlight;

		VkBuffer vertexBuffer;
		VkDeviceMemory vertexBufferMemory;
		VkBuffer stagingBuffer;
		VkDeviceMemory stagingBufferMemory;
		std::vector<Vertex> vertices = {
			{{0.0f, -0.5f}, {1.0f, 1.0f, 1.0f}},
			{{0.5f, 0.5f}, {0.0f, 1.0f, 0.0f}},
			{{-0.5f, 0.5f}, {0.0f, 0.0f, 1.0f}}
		};
		bool verticesDirty = true;
		size_t bufferSize;

		void refreshWindow(RefreshWindowEvent* event);
		void windowResize(WindowResizeEvent* event);
		void addVertices(AddVerticesEvent* event);
		void removeVertices(RemoveVerticesEvent* event);

		std::vector<VkSemaphore> imageAvailableSemaphores;
		std::vector<VkSemaphore> renderFinishedSemaphores;
		std::vector<VkFence> inFlightFences;
		std::vector<VkFence> imagesInFlight;
		size_t currentFrame = 0;

		void createBuffer(VkDeviceSize size, VkBufferUsageFlags usage, VkMemoryPropertyFlags properties, VkBuffer& buffer, VkDeviceMemory& bufferMemory);

		void createVertexBuffer(size_t size);
		uint32_t findMemoryType(uint32_t typeFilter, VkMemoryPropertyFlags properties);
		void fillVertexBuffer();
		void cleanupVertexBuffer();
	};
}
