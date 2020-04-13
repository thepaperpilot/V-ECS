#pragma once

#include <vector>

#include <GLFW/glfw3.h>
#define SOL_ALL_SAFETIES_ON 1
#include <sol\sol.hpp>

#include "EventManager.h"
#include "GLFWEvents.h"

namespace vecs {

	// An event can have listeners subscribe to it that will then run whenever the event if "fired"
	class LuaEvent {
	public:
		std::vector<sol::table> selves;
		std::vector<sol::function> listeners;

		void fire(sol::table event) {
			for (int i = 0; i < listeners.size(); i++) {
				auto result = listeners[i](selves[i], event);
				if (!result.valid()) {
					sol::error err = result;
					Debugger::addLog(DEBUG_LEVEL_ERROR, "[ERR] " + std::string(err.what()));
				}
			}
		}
	};

	// Wrapper class for letting lua access c++ events
	// Each c++ event must have a wrapper class that extends this,
	// with the appropriate callback method (and register that callback, probably in the constructor)
	class LuaEventWrapper {
	public:
		LuaEvent event;

		LuaEventWrapper(sol::state* lua) {
			this->lua = lua;
		}

	protected:
		sol::state* lua;
	};

	class WindowRefreshLuaEvent : public LuaEventWrapper {
	public:
		WindowRefreshLuaEvent(sol::state* lua) : LuaEventWrapper(lua) {
			EventManager::addListener(this, &WindowRefreshLuaEvent::callback);
		}

		void callback(RefreshWindowEvent* eventData) {
			event.fire(lua->create_table());
		}
	};

	class WindowResizeLuaEvent : public LuaEventWrapper {
	public:
		WindowResizeLuaEvent(sol::state* lua) : LuaEventWrapper(lua) {
			EventManager::addListener(this, &WindowResizeLuaEvent::callback);
		}

		void callback(WindowResizeEvent* eventData) {
			event.fire(lua->create_table_with(
				"width", eventData->width,
				"height", eventData->height
			));
		}
	};

	class MouseMoveLuaEvent : public LuaEventWrapper {
	public:
		MouseMoveLuaEvent(sol::state* lua) : LuaEventWrapper(lua) {
			EventManager::addListener(this, &MouseMoveLuaEvent::callback);
		}

		void callback(MouseMoveEvent* eventData) {
			event.fire(lua->create_table_with(
				"xPos", eventData->xPos,
				"yPos", eventData->yPos
			));
		}
	};

	// TODO convert mods to a table of booleans
	class LeftMousePressLuaEvent : public LuaEventWrapper {
	public:
		LeftMousePressLuaEvent(sol::state* lua) : LuaEventWrapper(lua) {
			EventManager::addListener(this, &LeftMousePressLuaEvent::callback);
		}

		void callback(LeftMousePressEvent* eventData) {
			event.fire(lua->create_table_with(
				"mods", eventData->mods
			));
		}
	};

	class LeftMouseReleaseLuaEvent : public LuaEventWrapper {
	public:
		LeftMouseReleaseLuaEvent(sol::state* lua) : LuaEventWrapper(lua) {
			EventManager::addListener(this, &LeftMouseReleaseLuaEvent::callback);
		}

		void callback(LeftMouseReleaseEvent* eventData) {
			event.fire(lua->create_table_with(
				"mods", eventData->mods
			));
		}
	};

	class RightMousePressLuaEvent : public LuaEventWrapper {
	public:
		RightMousePressLuaEvent(sol::state* lua) : LuaEventWrapper(lua) {
			EventManager::addListener(this, &RightMousePressLuaEvent::callback);
		}

		void callback(RightMousePressEvent* eventData) {
			event.fire(lua->create_table_with(
				"mods", eventData->mods
			));
		}
	};

	class RightMouseReleaseLuaEvent : public LuaEventWrapper {
	public:
		RightMouseReleaseLuaEvent(sol::state* lua) : LuaEventWrapper(lua) {
			EventManager::addListener(this, &RightMouseReleaseLuaEvent::callback);
		}

		void callback(RightMouseReleaseEvent* eventData) {
			event.fire(lua->create_table_with(
				"mods", eventData->mods
			));
		}
	};

	class VerticalScrollLuaEvent : public LuaEventWrapper {
	public:
		VerticalScrollLuaEvent(sol::state* lua) : LuaEventWrapper(lua) {
			EventManager::addListener(this, &VerticalScrollLuaEvent::callback);
		}

		void callback(VerticalScrollEvent* eventData) {
			event.fire(lua->create_table_with(
				"yOffset", eventData->yOffset
			));
		}
	};

	class HorizontalScrollLuaEvent : public LuaEventWrapper {
	public:
		HorizontalScrollLuaEvent(sol::state* lua) : LuaEventWrapper(lua) {
			EventManager::addListener(this, &HorizontalScrollLuaEvent::callback);
		}

		void callback(HorizontalScrollEvent* eventData) {
			event.fire(lua->create_table_with(
				"xOffset", eventData->xOffset
			));
		}
	};

	class KeyPressLuaEvent : public LuaEventWrapper {
	public:
		KeyPressLuaEvent(sol::state* lua) : LuaEventWrapper(lua) {
			EventManager::addListener(this, &KeyPressLuaEvent::callback);
		}

		void callback(KeyPressEvent* eventData) {
			event.fire(lua->create_table_with(
				"key", eventData->key,
				"scancode", eventData->scancode,
				"mods", eventData->mods
			));
		}
	};

	class KeyReleaseLuaEvent : public LuaEventWrapper {
	public:
		KeyReleaseLuaEvent(sol::state* lua) : LuaEventWrapper(lua) {
			EventManager::addListener(this, &KeyReleaseLuaEvent::callback);
		}

		void callback(KeyReleaseEvent* eventData) {
			event.fire(lua->create_table_with(
				"key", eventData->key,
				"scancode", eventData->scancode,
				"mods", eventData->mods
			));
		}
	};
}
