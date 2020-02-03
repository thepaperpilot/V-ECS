#pragma once

#include "EventManager.h"
#include "../rendering/Vertex.h"

#include <vector>

namespace vecs {

	struct AddVerticesEvent : public EventData {
		std::vector<Vertex> vertices;
	};
}
