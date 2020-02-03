#pragma once

#include <cstdint>
#include <typeindex>
#include <set>
#include <unordered_map>

namespace vecs {

	// Forward Declarations
	class World;
	struct Component;

	// This class describes a filter for finding only specific entities
	class EntityQuery {
	public:
		// Creates a query requiring all these component types
		EntityQuery(std::type_index components...);

		// Adds required types
		void with(std::type_index components...);
		// Adds disallowed types
		void without(std::type_index components...);

		// Checks if an entity matches the query
		bool checkEntity(World* world, uint32_t entity);
	private:
		std::set<std::type_index> required;
		std::set<std::type_index> disallowed;
	};
}
