#pragma once

#include <string>

#include "lua.hpp"
#include <LuaBridge/LuaBridge.h>

using namespace luabridge;

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
