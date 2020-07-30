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
		// Adds required types
		void with(std::string component);
		void with(std::vector<std::string> components);
		// Adds disallowed types
		void without(std::string component);
		void without(std::vector<std::string> components);

	private:
		std::set<std::string> required;
		std::set<std::string> disallowed;
	};

	// This becomes a container to track entities matching the filter
	class EntityQuery {
	public:
		ComponentFilter filter;
		ComponentFilter sharedFilter;

		std::vector<Archetype*> matchingArchetypes;
	};
}
