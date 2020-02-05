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
	class ComponentFilter {
	public:
		// Creates a query requiring all these component types
		ComponentFilter(std::type_index components...);

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

	// This struct becomes a container to track entities matching the filter
	struct EntityQuery {
		ComponentFilter filter;
		std::set<uint32_t> entities;
		std::function<void(uint32_t)> onEntityAdded;
		std::function<void(uint32_t)> onEntityRemoved;

		template <class T>
		static std::function<void(uint32_t)> bind(T* const object, void(T::* const mf)(uint32_t)) {
			return std::bind(mf, object, std::placeholders::_1);
		}
	};
}
