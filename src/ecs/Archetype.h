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

		sol::table getSharedComponent(std::string componentType);
		sol::table getComponentList(std::string componentType);

		bool checkQuery(EntityQuery* query);

		// Returns pair of data representing the first and last entity IDs
		std::pair<uint32_t, uint32_t> createEntities(uint32_t amount);

		void addEntities(std::vector<uint32_t> entities);
		void removeEntities(std::vector<uint32_t> entities);

	private:
		World* world;

		std::unordered_map<std::string, sol::table> components;
	};
}
