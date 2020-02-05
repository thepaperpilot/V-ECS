#include "VoxelWorld.h"
#include "../engine/Engine.h"
#include "../rendering/PostRenderSystem.h"

using namespace vecs;

void VoxelWorld::init() {
	// Add necessary systems
	addSystem(this->postRenderSystem = new PostRenderSystem(engine, maxFramesInFlight, initialVertexBufferSize), POST_RENDERING_PRIORITY);
}

void VoxelWorld::cleanup() {
	// Cleanup each system
	postRenderSystem->cleanup();
}
