#pragma once

#include <unordered_map>
#include <array>
#include <numeric>
#include <unordered_set>

#include <vulkan/vulkan.h>

#define SOL_ALL_SAFETIES_ON 1
#define SOL_DEFAULT_PASS_ON_ERROR 1
#include <sol\sol.hpp>

namespace vecs {

	// Forward Declarations
	class World;
	struct Component;
	struct EntityQuery;
	
	class Archetype {
	public:
		std::unordered_set<std::string> componentTypes;

		sol::table sharedComponents;

		Archetype(World* world, std::unordered_set<std::string> componentTypes, sol::table sharedComponents = sol::table());

		sol::table getComponent(std::string componentType, size_t index) {
			return components[componentType][index];
		}

		sol::table getSharedComponent(std::string componentType) {
			return sharedComponents[componentType];
		}

		sol::table getComponentList(std::string componentType);

		bool hasEntity(uint32_t entity);

		ptrdiff_t getIndex(uint32_t entity);

		bool checkQuery(EntityQuery* query);

		// Returns pair of data representing the first entity ID, and index within the archetype
		std::pair<uint32_t, size_t> createEntities(uint32_t amount);

		size_t addEntities(std::vector<uint32_t> entities);

		void removeEntities(std::vector<uint32_t> entities);

	private:
		World* world;

		std::vector<uint32_t> entities;

		std::unordered_map<std::string, sol::table> components;
	};
}
