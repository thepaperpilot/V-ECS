#pragma once

#include <string>
#include <filesystem>

#include "lua.hpp"
#include <LuaBridge/LuaBridge.h>
#include <LuaBridge/Vector.h>

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

	static lua_State* getState() {
		lua_State* L = luaL_newstate();
		luaL_openlibs(L);
		getGlobalNamespace(L)
			.addFunction("getFiles", &getResources)
			// TODO send this to a debug log somewhere
			.addFunction("print", &print);
		return L;
	}
}
