#include "VoxelWorld.h"
#include "../engine/Engine.h"
#include "systems/ChunkSystem.h"
#include "../movement/MovementSystem.h"
#include "../movement/PositionComponent.h"
#include "../movement/VelocityComponent.h"
#include "../input/ControllerSystem.h"
#include "../input/ControlledComponent.h"
#include "../rendering/PreRenderSystem.h"
#include "../rendering/MeshRendererSystem.h"
#include "../rendering/PostRenderSystem.h"

using namespace vecs;

void VoxelWorld::init() {
	// Add necessary systems
	addSystem(this->chunkSystem = new ChunkSystem, PRE_RENDERING_PRIORITY - 200);
	addSystem(this->controllerSystem = new ControllerSystem(engine->window), PRE_RENDERING_PRIORITY - 100); // Happens after chunk system
	addSystem(this->movementSystem = new MovementSystem, PRE_RENDERING_PRIORITY - 90); // Happens after controller system
	addSystem(this->preRenderSystem = new PreRenderSystem(engine), PRE_RENDERING_PRIORITY); // Prepares our image to draw to
	addSystem(this->meshRendererSystem = new MeshRendererSystem(engine), PRE_RENDERING_PRIORITY + POST_RENDERING_PRIORITY / 2); // Draws all our mesh components
	addSystem(this->postRenderSystem = new PostRenderSystem(engine), POST_RENDERING_PRIORITY); // Submits image to the GPU

	// Add player entity
	player = createEntity();
	addComponent(player, new VelocityComponent);
	addComponent(player, new PositionComponent);
	addComponent(player, new ControlledComponent);
}

void VoxelWorld::cleanup() {
	// Cleanup each system that needs it
	preRenderSystem->cleanup();
	meshRendererSystem->cleanup();
}
