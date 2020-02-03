#pragma once

namespace vecs {

	struct WindowResizeEvent : public EventData {
		int width;
		int height;
	};
}
