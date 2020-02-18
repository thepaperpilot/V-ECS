#pragma once

#include "World.h"

namespace vecs {
	
	// This is a class which has access to World's private members
	// World keeps track of dirty entities and has a bit some overhead
	// on some functions. Those are good, but sometimes when creating
	// large amounts of the same kind of entity (an entity "archetype"),
	// it can be beneficial to batch a lot of those things together and then
	// pay the overhead costs once for everything. 
	class ArchetypeBuilder {
	public:
		ArchetypeBuilder(World* world) {
			this->world = world;
		}

	protected:
		World* world;

		// Unsafe methods are methods intended for use in archetype builders
		// They remove some of the overhead costs but the implementation
		// must take care to perform them themselves
		uint32_t createEntities(int amount) {
			uint32_t firstEntity = world->nextEntity;
			world->nextEntity += amount;

			return firstEntity;
		};

		std::vector<EntityQuery*> getQueriesMatchingArchetype(std::set<std::type_index> componentTypes) {
			std::vector<EntityQuery*> queries;
			std::copy_if(world->queries.begin(), world->queries.end(), std::back_inserter(queries), [&componentTypes](EntityQuery* query) {
				return query->filter.checkArchetype(componentTypes);
			});
			return queries;
		};
	};
}
