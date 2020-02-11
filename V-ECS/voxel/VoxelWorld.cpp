#include "VoxelWorld.h"
#include "../engine/Engine.h"
#include "systems/ChunkSystem.h"
#include "../movement/MovementSystem.h"
#include "../movement/PositionComponent.h"
#include "../movement/VelocityComponent.h"
#include "../input/ControllerSystem.h"
#include "../input/ControlledComponent.h"
#include "rendering/CameraComponent.h"
#include "rendering/PushConstantComponent.h"
#include "rendering/MeshRendererSystem.h"
#include "rendering/CameraSystem.h"

#include <glm/gtc/matrix_transform.hpp>

using namespace vecs;

void VoxelWorld::init() {
	// Add necessary systems
	addSystem(this->chunkSystem = new ChunkSystem, 100);
	addSystem(this->controllerSystem = new ControllerSystem(window), 200); // Happens after chunk system
	addSystem(this->movementSystem = new MovementSystem, 300); // Happens after controller system
	addSystem(this->cameraSystem = new CameraSystem(&renderer), 400);
	addSystem(this->meshRendererSystem = new MeshRendererSystem(device, &voxelRenderer), START_RENDERING_PRIORITY + 100); // Draws all our mesh components

	// Add player entity
	player = createEntity();
	addComponent(player, new VelocityComponent);
	addComponent(player, new PositionComponent);
	addComponent(player, new ControlledComponent);
	addComponent(player, new CameraComponent);
	addComponent(player, new PushConstantComponent);

	// Register out voxel renderer
	voxelRenderer.init(this);
	renderer.registerSubRenderer(&voxelRenderer);
}

void VoxelWorld::cleanup() {
	// Cleanup each system that needs it
	meshRendererSystem->cleanup();
}
