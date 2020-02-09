#pragma once

#include "../ecs/System.h"
#include "../ecs/EntityQuery.h"

namespace vecs {

	// Forward declarations
	class Device;
	class Renderer;
	struct RefreshWindowEvent;
	struct WindowResizeEvent;

	class PreRenderSystem : public System {
	public:
		PreRenderSystem(Device* device, Renderer* renderer) {
			this->device = device;
			this->renderer = renderer;
		}

		void init() override;
		void update() override;
		void cleanup();

	private:
		Device* device;
		Renderer* renderer;

		EntityQuery renderState;
		void onRenderStateAdded(uint32_t entity);

		void refreshWindow(RefreshWindowEvent* event);
		void windowResize(WindowResizeEvent* event);
	};
}
