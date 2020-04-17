#include "LuaEvents.h"

#include "../ecs/World.h"

using namespace vecs;

void LuaEvent::fire(sol::table event) {
	for (int i = 0; i < listeners.size(); i++) {
		auto result = listeners[i](selves[i], world->config, event);
		if (!result.valid()) {
			sol::error err = result;
			Debugger::addLog(DEBUG_LEVEL_ERROR, "[ERR] " + std::string(err.what()));
		}
	}
}
