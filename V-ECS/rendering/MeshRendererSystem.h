#pragma once

#include "../ecs/System.h"
#include "../ecs/EntityQuery.h"
#include "MeshComponent.h"
#include <vulkan/vulkan.h>

namespace vecs {

	// Forward declarations
	class Device;
	class Renderer;

	class MeshRendererSystem : public System {
	public:
		MeshRendererSystem(Device* device, Renderer* renderer) {
			this->device = device;
			this->renderer = renderer;
		}

		bool framebufferResized = false;

		void init() override;
		void update() override;
		void cleanup();

	private:
		Device* device;
		Renderer* renderer;

		size_t initialVertexBufferSize = 4096;
		size_t initialIndexBufferSize = 4096;

		EntityQuery meshes;
		void onMeshAdded(uint32_t entity);

		void fillVertexBuffer(MeshComponent* mesh);
		void fillIndexBuffer(MeshComponent* mesh);
	};
}
