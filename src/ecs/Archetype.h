#pragma once

#include <atomic>
#include <unordered_map>
#include <array>
#include <numeric>
#include <set>
#include <unordered_set>
#include <shared_mutex>

#include <vulkan/vulkan.h>

#define SOL_ALL_SAFETIES_ON 1
#define SOL_DEFAULT_PASS_ON_ERROR 1
#include <sol\sol.hpp>

namespace vecs {

	// Forward Declarations
	class World;
	struct Component;
	class EntityQuery;
	class LuaVal;
	
	class Archetype {
	public:
		std::unordered_set<std::string> componentTypes;
		// TODO replace this with a bitset? (worse with a large number of small archetypes, better with small number of large archetypes)
		std::set<uint32_t> entities;

		LuaVal* sharedComponents;
		std::atomic_uint32_t numEntities = 0;
		std::shared_mutex mutex;

		Archetype(World* world, std::unordered_set<std::string> componentTypes, LuaVal* sharedComponents = nullptr);

		LuaVal getSharedComponent(std::string componentType);
		LuaVal getComponentList(std::string componentType);

		bool checkQuery(EntityQuery* query);

		// Returns pair of data representing the first entity's id and index within this archetype
		std::pair<uint32_t, uint32_t> createEntities(uint32_t amount);

		void addEntities(std::vector<uint32_t> entities);
		void removeEntities(std::vector<uint32_t> entities);
		void clearEntities();

		// Used for ensuring iterations over this entity don't occur whilst entities are added or removed
		// This is used because some jobs may iterate over entities and last between frames
		void lock_shared();
		void unlock_shared();

	private:
		World* world;

		std::unordered_map<std::string, LuaVal> components;
	};
}
