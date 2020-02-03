#pragma once

constexpr auto PRE_RENDERING_PRIORITY = 1000;
constexpr auto POST_RENDERING_PRIORITY = 2000;

#include <vulkan/vulkan.h>
#include "../ecs/World.h"

namespace vecs {

	// Forward Declarations
	class Engine;
	class VoxelRenderingSystem;
	class PostRenderSystem;

	// This is a type of World that comes with several built-in
	// systems for rendering a voxel-based world
	class VoxelWorld : public World {
	public:
		int maxFramesInFlight = 2;
		int initialVertexBufferSize = 4096;

		VoxelWorld(Engine* engine) {
			this->engine = engine;
		}

		// Overidden to setup up and cleanup our required systems
		void init() override;
		void cleanup() override;

	private:
		Engine* engine;

		VoxelRenderingSystem* voxelRenderingSystem;
		PostRenderSystem* postRenderSystem;
	};		
}
