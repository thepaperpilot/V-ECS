#pragma once

#include "../ecs/Archetype.h"
#include "../ecs/EntityQuery.h"
#include "../ecs/WorldLoadStatus.h"
#include "../engine/Buffer.h"
#include "../jobs/JobManager.h"
#include "../lib/FrustumCull.h"
#include "../rendering/SubRenderer.h"
#include "../rendering/Model.h"
#include "../rendering/Texture.h"

#define SOL_DEFAULT_PASS_ON_ERROR 1
#define SOL_ALL_SAFETIES_ON 1
#include <sol\sol.hpp>

#include <map>

#define GLM_FORCE_DEPTH_ZERO_TO_ONE
#include <glm/glm.hpp>
#include <HastyNoise/hastyNoise.h>

// Reference: https://gist.github.com/Rochet2/69c6b88070cf84ad7f547ded589db493

namespace vecs {

	enum LuaType {
		LUA_TYPE_NIL,
		LUA_TYPE_STRING,
		LUA_TYPE_TABLE,
		LUA_TYPE_BOOL,
		LUA_TYPE_NUMBER,
		LUA_TYPE_FUNCTION,
		// subset of usertypes, only the ones we want to support in components
		// the more we add the more complex our code and less performant it becomes
		LUA_TYPE_ARCHETYPE,
		LUA_TYPE_QUERY,
		LUA_TYPE_WORLD_LOAD_STATUS,
		LUA_TYPE_RENDERER,
		LUA_TYPE_JOB,
		LUA_TYPE_VEC2,
		LUA_TYPE_VEC3,
		LUA_TYPE_VEC4,
		LUA_TYPE_MAT4,
		LUA_TYPE_FRUSTUM,
		LUA_TYPE_NOISE,
		LUA_TYPE_MODEL,
		LUA_TYPE_TEXTURE,
		LUA_TYPE_BUFFER,
		LUA_TYPE_TEXT_FILTER,
		LUA_TYPE_FONT,
		LUA_TYPE_PIXELS
	};

	// Forward Declarations
	class LuaVal;

	class LuaVal {
	public:
		// type definition we use to store tables
		typedef std::map<LuaVal, LuaVal> MapType;

		// stores what type of value we have
		LuaType type;
		// type-safe version of a union, stores our actual value
		std::variant<
			double,
			bool,
			std::string,
			sol::bytecode,
			MapType*,
			Archetype*,
			EntityQuery*,
			WorldLoadStatus*,
			SubRenderer*,
			Job*,
			glm::vec2,
			glm::vec3,
			glm::vec4,
			glm::mat4*,
			Frustum*,
			HastyNoise::NoiseSIMD*,
			Model*,
			Texture*,
			Buffer*,
			ImGuiTextFilter*,
			ImFont*,
			unsigned char*,
			void*
		> value;

		// static functions for creating LuaVals from existing lua objects
		static LuaVal asLuaVal(sol::object const& v);
		static LuaVal fromTable(sol::table const& tb);

		// get and set based on key, for indexing values
		LuaVal get(std::string const& key) const;
		sol::object get_lua(sol::object const& key, sol::this_state const& s) const;
		void set(LuaVal const& key, LuaVal const& value) const;
		void set_nil(LuaVal const& key) const;
		void set_lua(sol::object const& key, sol::object const& val) const;
		bool contains(LuaVal const& key) const;
		int getLength() const;
		template<typename T, typename std::enable_if<std::is_enum<T>::value>::type * = nullptr>
		T getEnum() const {
			return static_cast<T>((int)std::get<double>(value));
		}
		void clear() const;

		bool operator<(LuaVal const& b) const;
		bool operator==(LuaVal const& b) const;
		bool operator!=(LuaVal const& b) const;
		std::string toString() const;

		// utility functions
		std::tuple<sol::object, MapType::iterator> iterate(sol::this_state const& s) const;
		std::tuple<sol::object, MapType::iterator> iterate_range(LuaVal start, LuaVal end, sol::this_state const& s) const;
		sol::object asObject(sol::this_state const& s) const;
		sol::object asTable(sol::this_state const& s) const;
		sol::object asLua(sol::this_state const& s) const;

		// constructors for various types
		LuaVal() : type(LUA_TYPE_NIL) {}
		LuaVal(std::string s) : value(s), type(LUA_TYPE_STRING) {}
		LuaVal(bool b) : value(b), type(LUA_TYPE_BOOL) {}
		LuaVal(double d) : value(d), type(LUA_TYPE_NUMBER) {}
		LuaVal(sol::function f) : value(f.dump()), type(LUA_TYPE_FUNCTION) {}
		LuaVal(MapType* t) : value(t), type(LUA_TYPE_TABLE) {}
		LuaVal(std::initializer_list<MapType::value_type> const& l) : value(new MapType(l)), type(LUA_TYPE_TABLE) {}
		LuaVal(Archetype* a) : value(a), type(LUA_TYPE_ARCHETYPE) {}
		LuaVal(EntityQuery* q) : value(q), type(LUA_TYPE_QUERY) {}
		LuaVal(WorldLoadStatus* w) : value(w), type(LUA_TYPE_WORLD_LOAD_STATUS) {}
		LuaVal(SubRenderer* s) : value(s), type(LUA_TYPE_RENDERER) {}
		LuaVal(Job* j) : value(j), type(LUA_TYPE_JOB) {}
		LuaVal(glm::vec2 v) : value(v), type(LUA_TYPE_VEC2) {}
		LuaVal(glm::vec3 v) : value(v), type(LUA_TYPE_VEC3) {}
		LuaVal(glm::vec4 v) : value(v), type(LUA_TYPE_VEC4) {}
		LuaVal(glm::mat4* v) : value(new glm::mat4(*v)), type(LUA_TYPE_MAT4) {}
		LuaVal(Frustum* f) : value(new Frustum(*f)), type(LUA_TYPE_FRUSTUM) {}
		LuaVal(HastyNoise::NoiseSIMD* n) : value(n), type(LUA_TYPE_NOISE) {}
		LuaVal(Model* m) : value(m), type(LUA_TYPE_MODEL) {}
		LuaVal(Texture* t) : value(t), type(LUA_TYPE_TEXTURE) {}
		LuaVal(Buffer* b) : value(new Buffer(*b)), type(LUA_TYPE_BUFFER) {}
		LuaVal(ImGuiTextFilter* f) : value(new ImGuiTextFilter(*f)), type(LUA_TYPE_TEXT_FILTER) {}
		LuaVal(ImFont* f) : value(f), type(LUA_TYPE_FONT) {}
		LuaVal(unsigned char* p) : value(p), type(LUA_TYPE_PIXELS) {}

	private:
		static LuaVal parseUserdata(sol::object const& v);
	};

	namespace LuaValBindings {

		void setupState(sol::state& lua);
	}

	size_t LuaValHash(vecs::LuaVal const& k);
}

namespace std {

	template <>
	struct hash<vecs::LuaVal> {
		std::size_t operator()(const vecs::LuaVal& k) const {
			return vecs::LuaValHash(k);
		}
	};
}
