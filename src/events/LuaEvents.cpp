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

bool WindowRefreshLuaEvent::callback(RefreshWindowEvent* eventData) {
	if (event.world->isDisposed) return false;
	if (!event.world->isValid) return true;
	event.fire(lua->create_table());
	return true;
}

bool WindowResizeLuaEvent::callback(WindowResizeEvent* eventData) {
	if (event.world->isDisposed) return false;
	if (!event.world->isValid) return true;
	event.fire(lua->create_table_with(
		"width", eventData->width,
		"height", eventData->height
	));
	return true;
}

bool MouseMoveLuaEvent::callback(MouseMoveEvent* eventData) {
	if (event.world->isDisposed) return false;
	if (!event.world->isValid) return true;
	event.fire(lua->create_table_with(
		"xPos", eventData->xPos,
		"yPos", eventData->yPos
	));
	return true;
}

bool LeftMousePressLuaEvent::callback(LeftMousePressEvent* eventData) {
	if (event.world->isDisposed) return false;
	if (!event.world->isValid) return true;
	event.fire(lua->create_table_with(
		"mods", eventData->mods
	));
	return true;
}

bool LeftMouseReleaseLuaEvent::callback(LeftMouseReleaseEvent* eventData) {
	if (event.world->isDisposed) return false;
	if (!event.world->isValid) return true;
	event.fire(lua->create_table_with(
		"mods", eventData->mods
	));
	return true;
}

bool RightMousePressLuaEvent::callback(RightMousePressEvent* eventData) {
	if (event.world->isDisposed) return false;
	if (!event.world->isValid) return true;
	event.fire(lua->create_table_with(
		"mods", eventData->mods
	));
	return true;
}

bool RightMouseReleaseLuaEvent::callback(RightMouseReleaseEvent* eventData) {
	if (event.world->isDisposed) return false;
	if (!event.world->isValid) return true;
	event.fire(lua->create_table_with(
		"mods", eventData->mods
	));
	return true;
}

bool VerticalScrollLuaEvent::callback(VerticalScrollEvent* eventData) {
	if (event.world->isDisposed) return false;
	if (!event.world->isValid) return true;
	event.fire(lua->create_table_with(
		"yOffset", eventData->yOffset
	));
	return true;
}

bool HorizontalScrollLuaEvent::callback(HorizontalScrollEvent* eventData) {
	if (event.world->isDisposed) return false;
	if (!event.world->isValid) return true;
	event.fire(lua->create_table_with(
		"xOffset", eventData->xOffset
	));
	return true;
}

bool KeyPressLuaEvent::callback(KeyPressEvent* eventData) {
	if (event.world->isDisposed) return false;
	if (!event.world->isValid) return true;
	event.fire(lua->create_table_with(
		"key", eventData->key,
		"scancode", eventData->scancode,
		"mods", eventData->mods
	));
	return true;
}

bool KeyReleaseLuaEvent::callback(KeyReleaseEvent* eventData) {
	if (event.world->isDisposed) return false;
	if (!event.world->isValid) return true;
	event.fire(lua->create_table_with(
		"key", eventData->key,
		"scancode", eventData->scancode,
		"mods", eventData->mods
	));
	return true;
}
