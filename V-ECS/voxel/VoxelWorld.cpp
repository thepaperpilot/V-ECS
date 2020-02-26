#include "VoxelWorld.h"
#include "../engine/Engine.h"
#include "../movement/MovementSystem.h"
#include "../movement/PositionComponent.h"
#include "../movement/VelocityComponent.h"
#include "../input/ControllerSystem.h"
#include "../input/ControlledComponent.h"
#include "data/ChunkSystem.h"
#include "rendering/CameraComponent.h"
#include "rendering/MeshRendererSystem.h"
#include "rendering/CameraSystem.h"

#include <glm/gtc/matrix_transform.hpp>

using namespace vecs;

void VoxelWorld::preInit() {
	// Add necessary systems
	addSystem(this->chunkSystem = new ChunkSystem(chunkSize), 100);
	addSystem(this->controllerSystem = new ControllerSystem(window), 200); // Happens after chunk system
	addSystem(this->movementSystem = new MovementSystem, 300); // Happens after controller system
	addSystem(this->cameraSystem = new CameraSystem(&voxelRenderer), 400);
	addSystem(this->meshRendererSystem = new MeshRendererSystem(device, &voxelRenderer), START_RENDERING_PRIORITY + 100); // Draws all our mesh components

	// Add player entity
	Archetype* archetype = getArchetype({ typeid(PositionComponent), typeid(VelocityComponent), typeid(ControlledComponent), typeid(CameraComponent) });
	std::pair<uint32_t, size_t> player = archetype->createEntities(1);
	this->player = player.first;
	archetype->getComponentList(typeid(PositionComponent))->at(player.second) = new PositionComponent;
	archetype->getComponentList(typeid(VelocityComponent))->at(player.second) = new VelocityComponent;
	archetype->getComponentList(typeid(ControlledComponent))->at(player.second) = new ControlledComponent;
	CameraComponent* camera = new CameraComponent;
	voxelRenderer.model = &camera->model;
	voxelRenderer.view = &camera->view;
	voxelRenderer.projection = &camera->projection;
	archetype->getComponentList(typeid(CameraComponent))->at(player.second) = camera;

	// Register our voxel renderer
	voxelRenderer.init(this);
	subrenderers.push_back(&voxelRenderer);
}
