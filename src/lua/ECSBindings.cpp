#include "ECSBindings.h"

#include "../ecs/Archetype.h"
#include "../ecs/EntityQuery.h"
#include "../ecs/World.h"
#include "../jobs/Worker.h"

void vecs::ECSBindings::setupState(sol::state& lua, Worker* worker) {
	lua.new_usertype<Archetype>("archetype",
		"new", sol::factories(
			[worker](std::unordered_set<std::string> required) -> Archetype* { return worker->getWorld()->getArchetype(required); },
			// I wish I didn't have to get a pointer for this, but I'd have circular dependencies if Archetype was allowed to have a non-pointer LuaVal
			[worker](std::unordered_set<std::string> required, sol::table sharedComponents) -> Archetype* { return worker->getWorld()->getArchetype(required, &LuaVal::fromTable(sharedComponents)); }
		),
		"isEmpty", [](Archetype& archetype) -> bool { return archetype.numEntities == 0; },
		"getComponents", &Archetype::getComponentList,
		"getSharedComponent", [](Archetype& archetype, std::string component_t) -> LuaVal { return archetype.getSharedComponent(component_t); },
		"createEntity", [](Archetype& archetype) -> std::pair<uint32_t, uint32_t> { return archetype.createEntities(1); },
		"createEntities", & Archetype::createEntities,
		"deleteEntity", [](Archetype& archetype, uint32_t entity) { archetype.removeEntities({ entity }); },
		"deleteEntities", & Archetype::removeEntities,
		"clearEntities", &Archetype::clearEntities,
		"lock_shared", &Archetype::lock_shared,
		"unlock_shared", &Archetype::unlock_shared,
		sol::meta_function::length, [](Archetype& archetype) -> uint32_t { return archetype.numEntities.load(); }
	);
	lua.new_usertype<EntityQuery>("query",
		"new", sol::factories(
			[worker](std::vector<std::string> required) -> EntityQuery* {
				EntityQuery* query = new EntityQuery();
				query->filter.with(required);
				worker->getWorld()->addQuery(query);
				return query;
			},
			[worker](std::vector<std::string> required, std::vector<std::string> disallowed) -> EntityQuery* {
				EntityQuery* query = new EntityQuery();
				query->filter.with(required);
				query->filter.without(disallowed);
				worker->getWorld()->addQuery(query);
				return query;
			}
		),
		"getArchetypes", [](const EntityQuery& query) -> sol::as_table_t<std::vector<Archetype*>> { return query.matchingArchetypes; }
	);
	// Note: note ideal for creating large amounts of similar entities. Use an archetype
	// This is just a convenience function for creating a single entity quickly
	// I don't create a new usertype because uint8_t is already marked non-constructible
	lua["createEntity"] = [worker](sol::table components) -> std::pair<uint32_t, uint32_t> {
		std::unordered_set<std::string> componentTypes;
		for (auto kvp : components) {
			componentTypes.insert(kvp.first.as<std::string>());
		}
		Archetype* archetype = worker->getWorld()->getArchetype(componentTypes);
		auto entity = archetype->createEntities(1);
		for (auto kvp : components) {
			archetype->getComponentList(kvp.first.as<std::string>()).set(LuaVal((double)entity.first), LuaVal::asLuaVal(kvp.second));
		}
		return entity;
	};
}
