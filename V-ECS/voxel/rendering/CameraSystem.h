#pragma once

#include "../../ecs/System.h"
#include "../../ecs/EntityQuery.h"

namespace vecs {

	// Forward declarations
	class Renderer;
	struct WindowResizeEvent;

	class CameraSystem : public System {
	public:
		CameraSystem(Renderer* renderer) {
			this->renderer = renderer;
		}

		void init() override;
		void update() override;

	private:
		Renderer* renderer;

		EntityQuery cameras;

		void windowResize(WindowResizeEvent* event);
		void onCameraAdded(uint32_t entity);
	};
}
