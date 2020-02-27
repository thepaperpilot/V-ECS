#include "Manifest.h"

#include "lua.hpp"
#include "lauxlib.h"
#include "lualib.h"
#include <LuaBridge/LuaBridge.h>
#include "../util/LuaUtils.h"

#include <vulkan/vulkan.h>

using namespace vecs;
using namespace luabridge;

Manifest::Manifest() {
    // Load Manifest
    lua_State* L = luaL_newstate();
    if (luaL_loadfile(L, "resources/manifest.lua") ||
        lua_pcall(L, 0, 0, 0)) {
        // TODO handle manifest not found
        return;
    }

    // Get application settings
    LuaRef app = getGlobal(L, "application");

    // Get (initial) window size
    windowWidth = getInt(app["width"], 1280);
    windowHeight = getInt(app["height"], 720);

    // Get (initial) application name
    applicationName = getString(app["name"]);

    // Get (optional) application version
    LuaRef version = app["version"];
    if (!version.isNil()) {
        applicationVersion = VK_MAKE_VERSION(
            getInt(version["major"]),
            getInt(version["minor"]),
            getInt(version["patch"]));
    }
}
