#pragma once

#include "../ecs/System.h"
#include "../ecs/EntityQuery.h"

namespace vecs {

	// Forward declarations
	class Engine;
	struct RefreshWindowEvent;
	struct WindowResizeEvent;

	class PreRenderSystem : public System {
	public:
		PreRenderSystem(Engine* engine) {
			this->engine = engine;
		}

		void init() override;
		void update() override;
		void cleanup();

	private:
		Engine* engine;

		EntityQuery renderState;
		void onRenderStateAdded(uint32_t entity);

		void refreshWindow(RefreshWindowEvent* event);
		void windowResize(WindowResizeEvent* event);
	};
}
