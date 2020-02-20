#pragma once

#include "../../ecs/System.h"
#include "../../ecs/EntityQuery.h"

namespace vecs {

	// Forward declarations
	class VoxelRenderer;
	struct WindowResizeEvent;

	class CameraSystem : public System {
	public:
		CameraSystem(VoxelRenderer* voxelRenderer) {
			this->voxelRenderer = voxelRenderer;
		}

		void init() override;
		void update() override;

	private:
		VoxelRenderer* voxelRenderer;

		EntityQuery cameras;

		void windowResize(WindowResizeEvent* event);
	};
}
