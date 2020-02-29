#pragma once

#include <string>
#include <filesystem>

#include "lua.hpp"
#include <LuaBridge/LuaBridge.h>
#include <LuaBridge/Vector.h>

#include <glm/glm.hpp>

#include "../lib/FrustumCull.h"
#include <glm\ext\matrix_transform.hpp>

using namespace luabridge;

namespace vecs {
	static std::string getString(LuaRef ref, std::string defaultString = "") {
		if (!ref.isString())
			return defaultString;
		return ref.cast<std::string>();
	}

	static int getInt(LuaRef ref, int defaultInt = 0) {
		if (!ref.isNumber())
			return defaultInt;
		return ref.cast<int>();
	}

	static std::vector<std::string> getResources(std::string path, std::string extension) {
		std::vector<std::string> resources;
		auto filepath = std::filesystem::path("./resources/" + path).make_preferred();
		for (const auto& entry : std::filesystem::directory_iterator(filepath))
			if (!entry.is_directory() && entry.path().extension() == extension)
				resources.push_back(entry.path().string());
		return resources;
	}

	static void print(std::string s) {
		std::cout << "[LUA] " << s << std::endl;
	}

	struct LuaMat4Handle {
		glm::mat4 mat4;

		LuaMat4Handle(glm::mat4 mat4) {
			this->mat4 = mat4;
		}

		void translate(float x, float y, float z) {
			mat4 = glm::translate(mat4, { x, y, z });
		}
	};
	
	struct LuaVec3Handle {
		glm::vec3 vec3;

		LuaVec3Handle(float x, float y, float z) {
			this->vec3 = { x, y, z };
		}

		LuaVec3Handle(glm::vec3 vec3) {
			this->vec3 = vec3;
		}

		LuaVec3Handle* add(LuaVec3Handle* other) {
			return new LuaVec3Handle(vec3 + other->vec3);
		}
	};

	struct LuaFrustumHandle {
		Frustum frustum;

		LuaFrustumHandle(glm::mat4 MVP) : frustum(MVP) {}

		bool isBoxVisible(LuaVec3Handle* minBounds, LuaVec3Handle* maxBounds) {
			return frustum.IsBoxVisible(minBounds->vec3, maxBounds->vec3);
		}
	};

	static lua_State* getState() {
		lua_State* L = luaL_newstate();
		luaL_openlibs(L);
		getGlobalNamespace(L)
			.addFunction("getFiles", &getResources)
			// TODO send this to a debug log somewhere
			.addFunction("print", &print)
			.beginClass<LuaMat4Handle>("mat4")
				.addFunction("translate", &LuaMat4Handle::translate)
			.endClass()
			.beginClass<LuaVec3Handle>("vec3")
				.addConstructor<void (*) (float , float, float)>()
				.addFunction("__add", &LuaVec3Handle::add)
			.endClass()
			.beginClass<LuaFrustumHandle>("frustum")
				.addFunction("isBoxVisible", &LuaFrustumHandle::isBoxVisible)
			.endClass();

		return L;
	}

	// This is used to bind enums to LuaBridge
	template <typename T>
	struct EnumWrapper {
		static typename std::enable_if<std::is_enum<T>::value, void>::type push(lua_State* L, T value) {
			lua_pushnumber(L, static_cast<std::size_t> (value));
		}

		static typename std::enable_if<std::is_enum<T>::value, T>::type get(lua_State* L, int index) {
			return static_cast <T> (lua_tointeger(L, index));
		}
	};
}
