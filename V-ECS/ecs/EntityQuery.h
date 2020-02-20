#pragma once

#include <cstdint>
#include <typeindex>
#include <set>
#include <unordered_map>
#include <unordered_set>
#include <functional>

namespace vecs {

	// Forward Declarations
	class World;
	class Archetype;
	struct Component;

	// This class describes a filter for finding only specific entities
	class ComponentFilter {
	public:
		// TODO should the with and without functions just use va_list?
		// Adds required types
		void with(std::type_index component);
		void with(std::vector<std::type_index> components);
		// Adds disallowed types
		void without(std::type_index component);
		void without(std::vector<std::type_index> components);

		// Checks if an entity matches the query
		bool checkEntity(World* world, uint32_t entity);
		bool checkArchetype(std::unordered_set<std::type_index> componentTypes);

	private:
		std::set<std::type_index> required;
		std::set<std::type_index> disallowed;
	};

	// This struct becomes a container to track entities matching the filter
	struct EntityQuery {
		ComponentFilter filter;

		std::vector<Archetype*> matchingArchetypes;
	};
}
