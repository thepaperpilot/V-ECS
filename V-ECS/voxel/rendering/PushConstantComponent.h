#pragma once

#include "Vertex.h"
#include "../../engine/Device.h"
#include "../../engine/Buffer.h"
#include "../../ecs/World.h"

#include <vector>

namespace vecs {

	struct PushConstant {
		uint32_t size;
		VkShaderStageFlags stageFlags;
		const void* data;
	};

	// A component that stores the data about a push constant to send to the GPU
	struct PushConstantComponent : Component {
		std::vector<PushConstant> constants;
	};
}
