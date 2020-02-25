#pragma once

#include "../ecs/World.h"
#include "rendering/VoxelRenderer.h"

#include <vulkan/vulkan.h>

// TODO implementation:
// It should add a component filter to find entities with BlockComponents
// Whenever one is added or removed it should modify its list of vertices
// and indices. It should share vertices between multiple blocks as possible,
// and remove any shared faces. You can ignore any vertex shared by 4 voxels.
// Whenever it needs to make a change, it should mark a flag that the vertices list
// has been changed, and a system that runs before post render should read that
// flag (this flag may be done by adding a specific component, but I'm not sure if
// we can make that trigger a system in the same frame) and remake the vertex and index buffers
// before postrender sends them to the GPU (and then remove the flag, of course)
// Later on non-block entities that also need to render would similarly set that flag to
// rebuild the buffers as needed

// Maybe find a way to implement flyweighting: e.g. only storing unique component values
// Maybe have entity manager map components to a component manager. Different component
// managers can then implement their own lookup functions to get the data for a specific
// entity, as well as their own function to modify said component.
// They can probably go in the same file the component struct is defined in

// I'd like to set it up using 3d chunking: probably 32x32x32 voxels, with a configurable
// chunk render and update distance. Modify PostRenderSystem to work with multiple allocations,
// and make each chunk its own allocation. That way whenever a block is add or removed, only
// that chunk needs to re-fill its vertex and index buffers. If a chunk goes out of the render
// distance, its entities can just be removed. If a chunk goes out of update distance, I think we
// can make a version of EntityQuery that automatically filters out entities not in that distance
// We may add entities for each chunk with a ChunkDataComponent, with a list of all entities in
// that chunk, so we can add filters to handle chunks being added or removed from the entity list
// Storing data would probably be one file for each chunk, or 32,768 voxels
// The max render distance would probably be a radius of ~7 chunks, for a total of 15x15x15 chunks,
// or a total of 110 million visible voxels (minus air). That would be ~250 visible blocks in all 6 directions
// Ideally chunk size should be a configurable value in VoxelWorld

// The voxels will be created by registering archetypes,
// which will have be a list of components and their default values
// Each archetype will have an id mapping to it, and files will store each block
// as an archetype id plus a map of data that is non-default

namespace vecs {

	// Forward Declarations
	class ChunkSystem;
	class ControllerSystem;
	class CameraSystem;
	class MovementSystem;
	class MeshRendererSystem;

	// This is a type of World that comes with several built-in
	// systems for rendering a voxel-based world
	class VoxelWorld : public World {
	public:
		VoxelRenderer voxelRenderer;

		VoxelWorld(Renderer* renderer, uint16_t chunkSize) : World(renderer) {
			this->chunkSize = chunkSize;
		};

		void preInit() override;

		void prewarm();

	private:
		uint16_t chunkSize;

		uint32_t player = 0;

		ChunkSystem* chunkSystem = nullptr;
		ControllerSystem* controllerSystem = nullptr;
		MovementSystem* movementSystem = nullptr;
		CameraSystem* cameraSystem = nullptr;
		MeshRendererSystem* meshRendererSystem = nullptr;
	};		
}
