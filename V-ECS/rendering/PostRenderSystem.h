#pragma once

#include "../ecs/System.h"
#include "../ecs/EntityQuery.h"
#include "MeshComponent.h"
#include "Vertex.h"
#include <vulkan/vulkan.h>
#include <GLFW/glfw3.h>
#include <vector>

namespace vecs {

	// Forward declarations
	class Engine;
	struct RefreshWindowEvent;
	struct WindowResizeEvent;

	class PostRenderSystem : public System {
	public:
		PostRenderSystem(Engine* engine);

		bool framebufferResized = false;

		void init() override;
		void update() override;
		void cleanup();

	private:
		// Pure ECS isn't completely necessary
		// I'm fine with having some information in a system,
		// if it isn't specific to any entity like these
		// No need for "singleton" components
		Engine* engine;
		uint32_t maxFramesInFlight = 2;
		size_t initialVertexBufferSize = 4096;
		size_t initialIndexBufferSize = 4096;

		EntityQuery meshes;

		void refreshWindow(RefreshWindowEvent* event);
		void windowResize(WindowResizeEvent* event);
		void onMeshAdded(uint32_t entity);

		std::vector<VkSemaphore> imageAvailableSemaphores;
		std::vector<VkSemaphore> renderFinishedSemaphores;
		std::vector<VkFence> inFlightFences;
		std::vector<VkFence> imagesInFlight;
		size_t currentFrame = 0;

		void createBuffer(VkDeviceSize size, VkBufferUsageFlags usage, VkMemoryPropertyFlags properties, VkBuffer& buffer, VkDeviceMemory& bufferMemory);
		uint32_t findMemoryType(uint32_t typeFilter, VkMemoryPropertyFlags properties);

		void createVertexBuffer(MeshComponent* mesh, size_t size);
		void fillVertexBuffer(MeshComponent* mesh);
		void cleanupVertexBuffer(MeshComponent* mesh);

		void createIndexBuffer(MeshComponent* mesh, size_t size);
		void fillIndexBuffer(MeshComponent* mesh);
		void cleanupIndexBuffer(MeshComponent* mesh);
	};
}
