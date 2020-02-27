#pragma once

#include <cstdint>
#include <typeindex>
#include <set>
#include <unordered_map>
#include <unordered_set>
#include <functional>

namespace vecs {

	// Forward Declarations
	class Archetype;

	// This class describes a filter for finding only specific entities
	class ComponentFilter {
	friend class Archetype;
	public:
		// TODO should the with and without functions just use va_list?
		// Adds required types
		void with(std::type_index component);
		void with(std::vector<std::type_index> components);
		// Adds disallowed types
		void without(std::type_index component);
		void without(std::vector<std::type_index> components);

	private:
		std::set<std::type_index> required;
		std::set<std::type_index> disallowed;
	};

	// This struct becomes a container to track entities matching the filter
	struct EntityQuery {
		ComponentFilter filter;
		ComponentFilter sharedFilter;

		std::vector<Archetype*> matchingArchetypes;
	};
}
