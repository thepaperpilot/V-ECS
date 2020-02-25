#pragma once

#include <map>
#include <string>

#include "../../ecs/World.h"
#include "../../rendering/Texture.h"

#include <vulkan/vulkan.h>

namespace vecs {

	// Forward Declarations
	struct Component;
	class Device;
	class Archetype;

	class BlockLoader {
	public:
		void init(World* world) {
			this->world = world;
		}

		VkDescriptorImageInfo loadBlocks(Device* device, VkQueue copyQueue);

		Archetype* getArchetype(std::string id);

		void cleanup();

	private:
		World* world;

		std::unordered_map<std::string, std::unordered_map<std::type_index, Component*>> sharedComponents;

		Texture stitchedTexture;
	};
}
