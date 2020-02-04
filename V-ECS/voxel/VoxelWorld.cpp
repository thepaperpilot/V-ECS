#include "VoxelWorld.h"
#include "../engine/Engine.h"
#include "../rendering/PostRenderSystem.h"
#include "VoxelRenderingSystem.h"

using namespace vecs;

void VoxelWorld::init() {
	// Add necessary systems
	addSystem(this->voxelRenderingSystem = new VoxelRenderingSystem, (PRE_RENDERING_PRIORITY + POST_RENDERING_PRIORITY) / 2);
	addSystem(this->postRenderSystem = new PostRenderSystem(engine, maxFramesInFlight, initialVertexBufferSize), POST_RENDERING_PRIORITY);
}

void VoxelWorld::cleanup() {
	// Cleanup each system
	postRenderSystem->cleanup();
}
