#include "VoxelWorld.h"
#include "ChunkSystem.h"
#include "../engine/Engine.h"
#include "../rendering/PostRenderSystem.h"

using namespace vecs;

void VoxelWorld::init() {
	// Add necessary systems
	addSystem(this->chunkSystem = new ChunkSystem, PRE_RENDERING_PRIORITY);
	addSystem(this->postRenderSystem = new PostRenderSystem(engine), POST_RENDERING_PRIORITY);
}

void VoxelWorld::cleanup() {
	// Cleanup each system
	postRenderSystem->cleanup();
}
