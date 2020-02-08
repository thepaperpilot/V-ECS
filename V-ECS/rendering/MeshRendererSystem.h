#pragma once

#include "../ecs/System.h"
#include "../ecs/EntityQuery.h"
#include "MeshComponent.h"
#include <vulkan/vulkan.h>

namespace vecs {

	// Forward declarations
	class Engine;

	class MeshRendererSystem : public System {
	public:
		MeshRendererSystem(Engine* engine) {
			this->engine = engine;
		}

		bool framebufferResized = false;

		void init() override;
		void update() override;
		void cleanup();

	private:
		Engine* engine;
		size_t initialVertexBufferSize = 4096;
		size_t initialIndexBufferSize = 4096;

		EntityQuery meshes;
		void onMeshAdded(uint32_t entity);

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
