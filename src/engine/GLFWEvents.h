#pragma once

#include "../events/EventManager.h"

namespace vecs {

	struct RefreshWindowEvent : EventData {};

	struct WindowResizeEvent : public EventData {
		int width;
		int height;
	};
}
