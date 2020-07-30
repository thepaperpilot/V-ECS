#include "LuaVal.h"

#include "../engine/Debugger.h"

using namespace vecs;

void vecs::LuaValBindings::setupState(sol::state& lua) {
	lua.new_usertype<LuaVal>("luaVal", sol::factories(&LuaVal::asLuaVal),
		"asTable", &LuaVal::asTable,
		"asLua", &LuaVal::asLua,
		"iterate", &LuaVal::iterate,
		"iterate_range", &LuaVal::iterate_range,
		sol::meta_function::index, &LuaVal::get_lua,
		sol::meta_function::new_index, &LuaVal::set_lua,
		sol::meta_function::length, &LuaVal::getLength
	);
}

LuaVal LuaVal::asLuaVal(sol::object const& v) {
	switch (v.get_type()) {
	case sol::type::boolean:
		return LuaVal(v.as<bool>());
	case sol::type::function:
		return LuaVal(v.as<sol::function>());
	case sol::type::lua_nil:
	case sol::type::none:
		return LuaVal();
	case sol::type::number:
		return LuaVal(v.as<double>());
	case sol::type::string:
		return LuaVal(v.as<std::string>());
	case sol::type::table:
		return fromTable(v);
	case sol::type::userdata:
		if (v.is<LuaVal>()) return v.as<LuaVal>();
		return parseUserdata(v);
	default:
		Debugger::addLog(DEBUG_LEVEL_WARN, "Attempting to create LuaVal with object of type " + (int)v.get_type());
		break;
	}
}

LuaVal LuaVal::fromTable(sol::table const& tb) {
	LuaVal v({});
	for (auto it : tb)
		v.set_lua(it.first, it.second);
	return v;
}

LuaVal LuaVal::get(std::string const& key) const {
	assert(type == LUA_TYPE_TABLE);
	auto& map = *std::get<MapType*>(value);
	auto it = map.find(key);
	if (it == map.end())
		return LuaVal();
	return it->second;
}

sol::object LuaVal::get_lua(sol::object const& key, sol::this_state const& s) const {
	assert(type == LUA_TYPE_TABLE);
	sol::state_view lua(s);
	auto& map = *std::get<MapType*>(value);
	auto klv = asLuaVal(key);
	auto it = map.find(klv);
	if (it == map.end())
		return sol::make_object(lua, sol::lua_nil);
	auto& val = it->second;
	return val.asObject(s);
}

void LuaVal::set(LuaVal const& key, LuaVal const& val) const {
	assert(type == LUA_TYPE_TABLE);
	assert(key.type != LUA_TYPE_NIL);
	if (val.type == LUA_TYPE_NIL)
		std::get<MapType*>(value)->erase(key);
	else
		(*std::get<MapType*>(value))[std::move(key)] = std::move(val);
}

void LuaVal::set_nil(LuaVal const& key) const {
	assert(type == LUA_TYPE_TABLE);
	std::get<MapType*>(value)->erase(key);
}

void LuaVal::set_lua(sol::object const& key, sol::object const& val) const {
	assert(type == LUA_TYPE_TABLE);
	auto kk = asLuaVal(key);
	auto vv = asLuaVal(val);
	assert(kk.type != LUA_TYPE_NIL);
	if (vv.type == LUA_TYPE_NIL)
		std::get<MapType*>(value)->erase(kk);
	else
		(*std::get<MapType*>(value))[std::move(kk)] = std::move(vv);
}

bool LuaVal::contains(LuaVal const& key) const {
	assert(type == LUA_TYPE_TABLE);
	auto table = std::get<MapType*>(value);
	return table->find(key) != table->end();
}

int LuaVal::getLength() const {
	assert(type == LUA_TYPE_TABLE);
	return std::get<MapType*>(value)->size();
}

void LuaVal::clear() const {
	assert(type == LUA_TYPE_TABLE);
	std::get<MapType*>(value)->clear();
}

bool LuaVal::operator<(LuaVal const&b) const {
	if (type < b.type) return true;
	if (type > b.type) return false;

	assert(type == LUA_TYPE_STRING || type == LUA_TYPE_BOOL || type == LUA_TYPE_NUMBER ||
		   type == LUA_TYPE_FUNCTION || type == LUA_TYPE_VEC2 || type == LUA_TYPE_VEC3 ||
		   type == LUA_TYPE_VEC4);

	// handle this based on type
	// we can't just do value < b.value because some types can't be directly compared
	switch (type) {
	case LUA_TYPE_STRING:
		return std::get<std::string>(value) < std::get<std::string>(b.value);
	case LUA_TYPE_BOOL:
		return std::get<bool>(value) < std::get<bool>(b.value);
	case LUA_TYPE_NUMBER:
		return std::get<double>(value) < std::get<double>(b.value);
	case LUA_TYPE_FUNCTION:
		return std::get<sol::bytecode>(value).as_string_view() < std::get<sol::bytecode>(b.value).as_string_view();
	case LUA_TYPE_VEC2:
		return glm::all(glm::lessThan(std::get<glm::vec2>(value), std::get<glm::vec2>(b.value)));
	case LUA_TYPE_VEC3:
		return glm::all(glm::lessThan(std::get<glm::vec3>(value), std::get<glm::vec3>(b.value)));
	case LUA_TYPE_VEC4:
		return glm::all(glm::lessThan(std::get<glm::vec4>(value), std::get<glm::vec4>(b.value)));
	default:
		return false;
	}
}

// TODO use hashes to make this faster
bool LuaVal::operator==(LuaVal const& b) const {
	// check type ourselves since it'll probably be faster than std::variant handling it
	if (type != b.type) return false;
	// handle this based on type
	// we can't just do value == b.value because some types can't be directly compared
	assert(type != LUA_TYPE_NIL);
	switch (type) {
	case LUA_TYPE_STRING: return std::get<std::string>(value) == std::get<std::string>(b.value);
	case LUA_TYPE_TABLE: {
		LuaVal::MapType aMap = *std::get<LuaVal::MapType*>(value);
		LuaVal::MapType bMap = *std::get<LuaVal::MapType*>(b.value);
		if (aMap.size() != bMap.size()) return false;
		for (auto kvp : aMap) {
			if (bMap.count(kvp.first) == 0) return false;
			if (kvp.second != bMap[kvp.first]) return false;
		}
		return true;
	}
	case LUA_TYPE_BOOL: return std::get<bool>(value) == std::get<bool>(b.value);
	case LUA_TYPE_NUMBER: return std::get<double>(value) == std::get<double>(b.value);
	case LUA_TYPE_FUNCTION:
		return std::get<sol::bytecode>(value).as_string_view() == std::get<sol::bytecode>(b.value).as_string_view();
	case LUA_TYPE_ARCHETYPE: return std::get<Archetype*>(value) == std::get<Archetype*>(b.value);
	case LUA_TYPE_QUERY: return std::get<EntityQuery*>(value) == std::get<EntityQuery*>(b.value);
	case LUA_TYPE_WORLD_LOAD_STATUS: return std::get<WorldLoadStatus*>(value) == std::get<WorldLoadStatus*>(b.value);
	case LUA_TYPE_RENDERER: return std::get<SubRenderer*>(value) == std::get<SubRenderer*>(b.value);
	case LUA_TYPE_JOB: return std::get<Job*>(value) == std::get<Job*>(b.value);
	case LUA_TYPE_VEC2:
		return glm::all(glm::equal(std::get<glm::vec2>(value), std::get<glm::vec2>(b.value)));
	case LUA_TYPE_VEC3:
		return glm::all(glm::equal(std::get<glm::vec3>(value), std::get<glm::vec3>(b.value)));
	case LUA_TYPE_VEC4:
		return glm::all(glm::equal(std::get<glm::vec4>(value), std::get<glm::vec4>(b.value)));
	case LUA_TYPE_MAT4:
		// there is no glm::lessThan for matrix types, so they're just always false
		// TODO find a way to compare them
		return false;
	case LUA_TYPE_FRUSTUM: return std::get<Frustum*>(value) == std::get<Frustum*>(b.value);
	case LUA_TYPE_NOISE: return std::get<HastyNoise::NoiseSIMD*>(value) == std::get<HastyNoise::NoiseSIMD*>(b.value);
	case LUA_TYPE_MODEL: return std::get<Model*>(value) == std::get<Model*>(b.value);
	case LUA_TYPE_TEXTURE: return std::get<Texture*>(value) == std::get<Texture*>(b.value);
	case LUA_TYPE_BUFFER: return std::get<Buffer*>(value) == std::get<Buffer*>(b.value);
	case LUA_TYPE_TEXT_FILTER: return std::get<ImGuiTextFilter*>(value) == std::get<ImGuiTextFilter*>(b.value);
	case LUA_TYPE_FONT: return std::get<ImFont*>(value) == std::get<ImFont*>(b.value);
	case LUA_TYPE_PIXELS: return std::get<unsigned char*>(value) == std::get<unsigned char*>(b.value);
	default: return false;
	}
}

bool LuaVal::operator!=(LuaVal const& b) const {
	return !(*this == b);
}

std::string LuaVal::toString() const {
	switch (type) {
	case LUA_TYPE_NIL:
		return "nil";
	case LUA_TYPE_STRING:
		return std::get<std::string>(value);
	case LUA_TYPE_BOOL:
		return std::get<bool>(value) ? "true" : "false";
	case LUA_TYPE_NUMBER:
		return std::to_string(std::get<double>(value));
	// TODO cases for userdata
	default:
		return "Userdata";
	}
}

std::tuple<sol::object, LuaVal::MapType::iterator> LuaVal::iterate(sol::this_state const& s) const {
	assert(type == LUA_TYPE_TABLE);
	auto func = [s, end = std::get<MapType*>(value)->end()](MapType::iterator& it)->std::tuple<sol::object, sol::object> {
		if (it == end) {
			sol::state_view lua(s);
			return { sol::make_object(lua, sol::lua_nil), sol::make_object(lua, sol::lua_nil) };
		}
		auto oldit = it++;
		return std::make_tuple(oldit->first.asObject(s), oldit->second.asObject(s));
	};
	return std::make_tuple(sol::make_object(sol::state_view(s), func), std::get<MapType*>(value)->begin());
}

std::tuple<sol::object, LuaVal::MapType::iterator> LuaVal::iterate_range(LuaVal start, LuaVal end, sol::this_state const& s) const {
	assert(type == LUA_TYPE_TABLE);
	LuaVal::MapType* map = std::get<MapType*>(value);

	LuaVal::MapType::iterator beginIter = map->begin();
	while (beginIter != map->end() && beginIter->first < start)
		beginIter++;

	auto func = [s, end, mEnd = map->end()](MapType::iterator& it)->std::tuple<sol::object, sol::object> {
		if (it != mEnd && it->first < end) {
			auto oldit = it++;
			return std::make_tuple(oldit->first.asObject(s), oldit->second.asObject(s));
		}
		sol::state_view lua(s);
		return { sol::make_object(lua, sol::lua_nil), sol::make_object(lua, sol::lua_nil) };		
	};
	return std::make_tuple(sol::make_object(sol::state_view(s), func), beginIter);
}

sol::object LuaVal::asObject(sol::this_state const& s) const {
	sol::state_view lua(s);
	switch (type) {
	case LUA_TYPE_NIL: return sol::make_object(lua, sol::lua_nil);
	case LUA_TYPE_STRING: return sol::make_object(lua, std::get<std::string>(value));
	case LUA_TYPE_TABLE: return sol::make_object(lua, *this);
	case LUA_TYPE_BOOL: return sol::make_object(lua, std::get<bool>(value));
	case LUA_TYPE_NUMBER: return sol::make_object(lua, std::get<double>(value));
	case LUA_TYPE_FUNCTION: return lua.load(std::get<sol::bytecode>(value).as_string_view());
	case LUA_TYPE_ARCHETYPE: return sol::make_object(lua, std::get<Archetype*>(value));
	case LUA_TYPE_QUERY: return sol::make_object(lua, std::get<EntityQuery*>(value));
	case LUA_TYPE_WORLD_LOAD_STATUS: return sol::make_object(lua, std::get<WorldLoadStatus*>(value));
	case LUA_TYPE_RENDERER: return sol::make_object(lua, std::get<SubRenderer*>(value));
	case LUA_TYPE_JOB: return sol::make_object(lua, std::get<Job*>(value));
	case LUA_TYPE_VEC2: return sol::make_object(lua, std::get<glm::vec2>(value));
	case LUA_TYPE_VEC3: return sol::make_object(lua, std::get<glm::vec3>(value));
	case LUA_TYPE_VEC4: return sol::make_object(lua, std::get<glm::vec4>(value));
	case LUA_TYPE_MAT4: return sol::make_object(lua, std::get<glm::mat4*>(value));
	case LUA_TYPE_FRUSTUM: return sol::make_object(lua, std::get<Frustum*>(value));
	case LUA_TYPE_NOISE: return sol::make_object(lua, std::get<HastyNoise::NoiseSIMD*>(value));
	case LUA_TYPE_MODEL: return sol::make_object(lua, std::get<Model*>(value));
	case LUA_TYPE_TEXTURE: return sol::make_object(lua, std::get<Texture*>(value));
	case LUA_TYPE_BUFFER: return sol::make_object(lua, std::get<Buffer*>(value));
	case LUA_TYPE_TEXT_FILTER: return sol::make_object(lua, std::get<ImGuiTextFilter*>(value));
	case LUA_TYPE_FONT: return sol::make_object(lua, std::get<ImFont*>(value));
	case LUA_TYPE_PIXELS: return sol::make_object(lua, std::get<unsigned char*>(value));
	}
	return sol::make_object(lua, sol::lua_nil);
}

sol::object LuaVal::asTable(sol::this_state const& s) const {
	if (type == LUA_TYPE_TABLE) {
		sol::state_view lua(s);
		auto tbl = lua.create_table();
		for (auto& it : *std::get<MapType*>(value)) {
			it.first;
			tbl.raw_set(it.first.asObject(s), it.second.asObject(s));
		}
		return tbl;
	}
	return asObject(s);
}

sol::object LuaVal::asLua(sol::this_state const& s) const {
	if (type == LUA_TYPE_TABLE) {
		sol::state_view lua(s);
		auto tbl = lua.create_table();
		for (auto& it : *std::get<MapType*>(value)) {
			it.first;
			tbl.raw_set(it.first.asLua(s), it.second.asLua(s));
		}
		return tbl;
	}
	return asObject(s);
}

LuaVal LuaVal::parseUserdata(sol::object const& v) {
	sol::object type = v.as<sol::table>()["__type"]["name"];
	if (type.get_type() == sol::type::string) {
		std::string s = type.as<std::string>();
		if (s == "vecs::Archetype") return LuaVal(v.as<Archetype*>());
		else if (s == "vecs::EntityQuery") return LuaVal(v.as<EntityQuery*>());
		else if (s == "vecs::WorldLoadStatus") return LuaVal(v.as<WorldLoadStatus*>());
		else if (s == "vecs::SubRenderer") return LuaVal(v.as<SubRenderer*>());
		else if (s == "vecs::Job") return LuaVal(v.as<Job*>());
		else if (s == "glm::vec<2,float,0>") return LuaVal(v.as<glm::vec2>());
		else if (s == "glm::vec<3,float,0>") return LuaVal(v.as<glm::vec3>());
		else if (s == "glm::vec<4,float,0>") return LuaVal(v.as<glm::vec4>());
		else if (s == "glm::mat<4,4,float,0>") return LuaVal(v.as<glm::mat4*>());
		else if (s == "Frustum") return LuaVal(v.as<Frustum*>());
		else if (s == "HastyNoise::NoiseSIMD") return LuaVal(v.as<HastyNoise::NoiseSIMD*>());
		else if (s == "vecs::Model") return LuaVal(v.as<Model*>());
		else if (s == "vecs::Texture") return LuaVal(v.as<Texture*>());
		else if (s == "vecs::Buffer") return LuaVal(v.as<Buffer*>());
		else if (s == "ImGuiTextFilter") return LuaVal(v.as<ImGuiTextFilter*>());
		else if (s == "ImFont") return LuaVal(v.as<ImFont*>());
		else if (s == "unsigned char") return LuaVal(v.as<unsigned char*>());
		Debugger::addLog(DEBUG_LEVEL_WARN, "Attempting to create LuaVal with object of userdata type " + s);
	}
	// Fallback
	return LuaVal();
}

inline size_t vecs::LuaValHash(LuaVal const& k) {
	// TODO handle non-pointer values
	return std::hash<void*>{}(std::get<void*>(k.value));
}
