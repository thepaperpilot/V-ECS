#pragma once

#include "../ecs/System.h"
#include "../ecs/EntityQuery.h"

namespace vecs {

	// Forward declarations
	class Device;
	class Renderer;

	class PostRenderSystem : public System {
	public:
		PostRenderSystem(Device* device, Renderer* renderer) {
			this->device = device;
			this->renderer = renderer;
		}

		void init() override;
		void update() override;

	private:
		Device* device;
		Renderer* renderer;

		EntityQuery renderState;
	};
}
