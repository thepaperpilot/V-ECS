#pragma once

#include <vector>

#include <GLFW/glfw3.h>
#define SOL_DEFAULT_PASS_ON_ERROR 1
#define SOL_ALL_SAFETIES_ON 1
#include <sol\sol.hpp>

#include "EventManager.h"
#include "GLFWEvents.h"

namespace vecs {

	// Forward Declarations
	class World;

	// An event can have listeners subscribe to it that will then run whenever the event if "fired"
	class LuaEvent {
	public:
		std::vector<sol::table> selves;
		std::vector<sol::function> listeners;
		World* world;

		void fire(sol::table event);
	};

	// Wrapper class for letting lua access c++ events
	// Each c++ event must have a wrapper class that extends this,
	// with the appropriate callback method (and register that callback, probably in the constructor)
	// The callback should check if the world is disposed, and if so remove the listener
	// Additionally if the world isn't valid then the world may still be being built in another thread,
	// in which case using the lua object may cause problems, so its recommended to just ignore
	// the event in that case
	class LuaEventWrapper {
	public:
		LuaEvent event;

		LuaEventWrapper(sol::state* lua, World* world) {
			this->lua = lua;
			event.world = world;
		}

	protected:
		sol::state* lua;
	};

	class WindowRefreshLuaEvent : public LuaEventWrapper {
	public:
		WindowRefreshLuaEvent(sol::state* lua, World* world) : LuaEventWrapper(lua, world) {
			EventManager::addListener(this, &WindowRefreshLuaEvent::callback);
		}

		bool callback(RefreshWindowEvent* eventData);
	};

	class WindowResizeLuaEvent : public LuaEventWrapper {
	public:
		WindowResizeLuaEvent(sol::state* lua, World* world) : LuaEventWrapper(lua, world) {
			EventManager::addListener(this, &WindowResizeLuaEvent::callback);
		}

		bool callback(WindowResizeEvent* eventData);
	};

	class MouseMoveLuaEvent : public LuaEventWrapper {
	public:
		MouseMoveLuaEvent(sol::state* lua, World* world) : LuaEventWrapper(lua, world) {
			EventManager::addListener(this, &MouseMoveLuaEvent::callback);
		}

		bool callback(MouseMoveEvent* eventData);
	};

	// TODO convert mods to a table of booleans
	class LeftMousePressLuaEvent : public LuaEventWrapper {
	public:
		LeftMousePressLuaEvent(sol::state* lua, World* world) : LuaEventWrapper(lua, world) {
			EventManager::addListener(this, &LeftMousePressLuaEvent::callback);
		}

		bool callback(LeftMousePressEvent* eventData);
	};

	class LeftMouseReleaseLuaEvent : public LuaEventWrapper {
	public:
		LeftMouseReleaseLuaEvent(sol::state* lua, World* world) : LuaEventWrapper(lua, world) {
			EventManager::addListener(this, &LeftMouseReleaseLuaEvent::callback);
		}

		bool callback(LeftMouseReleaseEvent* eventData);
	};

	class RightMousePressLuaEvent : public LuaEventWrapper {
	public:
		RightMousePressLuaEvent(sol::state* lua, World* world) : LuaEventWrapper(lua, world) {
			EventManager::addListener(this, &RightMousePressLuaEvent::callback);
		}

		bool callback(RightMousePressEvent* eventData);
	};

	class RightMouseReleaseLuaEvent : public LuaEventWrapper {
	public:
		RightMouseReleaseLuaEvent(sol::state* lua, World* world) : LuaEventWrapper(lua, world) {
			EventManager::addListener(this, &RightMouseReleaseLuaEvent::callback);
		}

		bool callback(RightMouseReleaseEvent* eventData);
	};

	class VerticalScrollLuaEvent : public LuaEventWrapper {
	public:
		VerticalScrollLuaEvent(sol::state* lua, World* world) : LuaEventWrapper(lua, world) {
			EventManager::addListener(this, &VerticalScrollLuaEvent::callback);
		}

		bool callback(VerticalScrollEvent* eventData);
	};

	class HorizontalScrollLuaEvent : public LuaEventWrapper {
	public:
		HorizontalScrollLuaEvent(sol::state* lua, World* world) : LuaEventWrapper(lua, world) {
			EventManager::addListener(this, &HorizontalScrollLuaEvent::callback);
		}

		bool callback(HorizontalScrollEvent* eventData);
	};

	class KeyPressLuaEvent : public LuaEventWrapper {
	public:
		KeyPressLuaEvent(sol::state* lua, World* world) : LuaEventWrapper(lua, world) {
			EventManager::addListener(this, &KeyPressLuaEvent::callback);
		}

		bool callback(KeyPressEvent* eventData);
	};

	class KeyReleaseLuaEvent : public LuaEventWrapper {
	public:
		KeyReleaseLuaEvent(sol::state* lua, World* world) : LuaEventWrapper(lua, world) {
			EventManager::addListener(this, &KeyReleaseLuaEvent::callback);
		}

		bool callback(KeyReleaseEvent* eventData);
	};
}
