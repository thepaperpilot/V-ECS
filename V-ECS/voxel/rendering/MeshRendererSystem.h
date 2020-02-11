#pragma once

#include "../../ecs/System.h"
#include "../../ecs/EntityQuery.h"
#include "MeshComponent.h"
#include <vulkan/vulkan.h>

namespace vecs {

	// Forward declarations
	class Device;
	class VoxelRenderer;

	class MeshRendererSystem : public System {
	public:
		MeshRendererSystem(Device* device, VoxelRenderer* voxelRenderer) {
			this->device = device;
			this->voxelRenderer = voxelRenderer;
		}

		bool framebufferResized = false;

		void init() override;
		void update() override;
		void cleanup();

	private:
		Device* device;
		VoxelRenderer* voxelRenderer;

		size_t initialVertexBufferSize = 4096;
		size_t initialIndexBufferSize = 4096;

		EntityQuery meshes;
		void onMeshAdded(uint32_t entity);

		void fillVertexBuffer(MeshComponent* mesh);
		void fillIndexBuffer(MeshComponent* mesh);
	};
}
