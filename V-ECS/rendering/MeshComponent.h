#pragma once

#include "../ecs/World.h"
#include "../rendering/Vertex.h"

namespace vecs {

	// A component that stores the vertex and index buffers for
	// a piece of geometry
	struct MeshComponent : Component {
		const std::vector<Vertex> vertices;
		const std::vector<uint16_t> indices;
	};
}
