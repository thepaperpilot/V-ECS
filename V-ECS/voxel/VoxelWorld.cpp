#include "VoxelWorld.h"
#include "../engine/Engine.h"
#include "systems/ChunkSystem.h"
#include "../movement/MovementSystem.h"
#include "../movement/PositionComponent.h"
#include "../movement/VelocityComponent.h"
#include "../input/ControllerSystem.h"
#include "../input/ControlledComponent.h"
#include "rendering/CameraComponent.h"
#include "rendering/MeshRendererSystem.h"
#include "rendering/CameraSystem.h"

#include <glm/gtc/matrix_transform.hpp>

using namespace vecs;

VoxelWorld::VoxelWorld(uint16_t chunkSize) {
	// We add this system immediately so we can use our chunk archetype builder
	// before starting the game. Technically that's a temporary thing until I
	// make the proc gen system, but even then since this is the only system
	// that cares about chunk size, I might still register this system here
	addSystem(this->chunkSystem = new ChunkSystem(chunkSize), 100);
}

void VoxelWorld::init() {
	// Add necessary systems
	addSystem(this->controllerSystem = new ControllerSystem(window), 200); // Happens after chunk system
	addSystem(this->movementSystem = new MovementSystem, 300); // Happens after controller system
	addSystem(this->cameraSystem = new CameraSystem(&voxelRenderer), 400);
	addSystem(this->meshRendererSystem = new MeshRendererSystem(device, &voxelRenderer), START_RENDERING_PRIORITY + 100); // Draws all our mesh components

	// Add player entity
	player = createEntity();
	addComponent(player, new VelocityComponent);
	addComponent(player, new PositionComponent);
	addComponent(player, new ControlledComponent);
	addComponent(player, new CameraComponent);

	// Register out voxel renderer
	voxelRenderer.init(this);
	renderer.registerSubRenderer(&voxelRenderer);
}

void VoxelWorld::cleanupSystems() {
	// Cleanup each system that needs it
	meshRendererSystem->cleanup();
}
