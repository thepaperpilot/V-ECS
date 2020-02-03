#pragma once

#include "../rendering/Vertex.h"
#include "../ecs/System.h"

#include <vector>

namespace vecs {
	
	// This is a class that will render every BlockComponent
	// TODO implementation:
	// It should add a component filter to find entities with BlockComponents
	// Whenever one is added or removed it should modify its list of vertices
	// and indices. It should share vertices between multiple blocks as possible.
	// Whenever it needs to make a change, it should mark a flag that the vertices list
	// has been changed, and a system that runs before post render should read that
	// flag (this flag may be done by adding a specific component, but I'm not sure if
	// we can make that trigger a system in the same frame) and remake the vertex and index buffers
	// before postrender sends them to the GPU (and then remove the flag, of course)
	// Later on non-block entities that also need to render would similarly set that flag to
	// rebuild the buffers as needed
	class VoxelRenderingSystem : public System {
	public:
		void update() override;

	private:
	};
}
