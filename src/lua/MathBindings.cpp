#include "MathBindings.h"

#include "../lib/FrustumCull.h"

#include <glm/glm.hpp>
#include <glm/ext/matrix_clip_space.hpp>
#include <glm/gtx/string_cast.hpp>

void vecs::MathBindings::setupState(sol::state& lua) {
	// glm functions, w/o namespace to make it more glsl-like
	lua["normalize"] = [](glm::vec3 v) -> glm::vec3 { return glm::normalize(v); };
	lua["radians"] = [](float v) -> float { return glm::radians(v); };
	lua["sin"] = [](float v) -> float { return glm::sin(v); };
	lua["cos"] = [](float v) -> float { return glm::cos(v); };
	lua["cross"] = [](glm::vec3 first, glm::vec3 second) -> glm::vec3 { return glm::cross(first, second); };
	lua["perspective"] = [](float fov, float aspectRatio, float nearPlane, float farPlane) -> glm::mat4 { return glm::perspective(fov, aspectRatio, nearPlane, farPlane); };
	lua["lookAt"] = [](glm::vec3 position, glm::vec3 forward, glm::vec3 up) -> glm::mat4 { return glm::lookAt(position, forward, up); };
	lua.new_usertype<glm::vec2>("vec2",
		sol::constructors<glm::vec2(), glm::vec2(float), glm::vec2(float, float)>(),
		"x", &glm::vec2::x,
		"y", &glm::vec2::y,
		"length", [](const glm::vec2& v1) -> float { return glm::length(v1); },
		sol::meta_function::multiplication, sol::overload(
			[](const glm::vec2& v1, const glm::vec2& v2) -> glm::vec2 { return v1 * v2; },
			[](const glm::vec2& v1, float f) -> glm::vec2 { return v1 * f; },
			[](float f, const glm::vec2& v1) -> glm::vec2 { return f * v1; }
		),
		sol::meta_function::addition, [](const glm::vec2& v1, const glm::vec2& v2) -> glm::vec2 { return v1 + v2; },
		sol::meta_function::subtraction, [](const glm::vec2& v1, const glm::vec2& v2) -> glm::vec2 { return v1 - v2; },
		sol::meta_function::to_string, [](const glm::vec2& v1) -> std::string { return glm::to_string(v1); }
	);
	lua.new_usertype<glm::vec3>("vec3",
		sol::constructors<glm::vec3(), glm::vec3(float), glm::vec3(float, float, float)>(),
		"x", &glm::vec3::x,
		"y", &glm::vec3::y,
		"z", &glm::vec3::z,
		"length", [](const glm::vec3& v1) -> float { return glm::length(v1); },
		"normalized", [](const glm::vec3& v1) -> glm::vec3 { return glm::normalize(v1); },
		sol::meta_function::multiplication, sol::overload(
			[](const glm::vec3& v1, const glm::vec3& v2) -> glm::vec3 { return v1 * v2; },
			[](const glm::vec3& v1, float f) -> glm::vec3 { return v1 * f; },
			[](const glm::vec3& v1, float f) -> glm::vec3 { return f * v1; }
		),
		sol::meta_function::addition, [](const glm::vec3& v1, const glm::vec3& v2) -> glm::vec3 { return v1 + v2; },
		sol::meta_function::subtraction, [](const glm::vec3& v1, const glm::vec3& v2) -> glm::vec3 { return v1 - v2; },
		sol::meta_function::to_string, [](const glm::vec3& v1) -> std::string { return glm::to_string(v1); }
	);
	lua.new_usertype<glm::vec4>("vec4",
		sol::constructors<glm::vec4(), glm::vec4(float), glm::vec4(float, float, float, float)>(),
		"w", &glm::vec4::w,
		"x", &glm::vec4::x,
		"y", &glm::vec4::y,
		"z", &glm::vec4::z,
		"length", [](const glm::vec4& v1) -> float { return glm::length(v1); },
		sol::meta_function::multiplication, sol::overload(
			[](const glm::vec4& v1, const glm::vec4& v2) -> glm::vec4 { return v1 * v2; },
			[](const glm::vec4& v1, float f) -> glm::vec4 { return v1 * f; },
			[](float f, const glm::vec4& v1) -> glm::vec4 { return f * v1; }
		),
		sol::meta_function::addition, [](const glm::vec4& v1, const glm::vec4& v2) -> glm::vec4 { return v1 + v2; },
		sol::meta_function::subtraction, [](const glm::vec4& v1, const glm::vec4& v2) -> glm::vec4 { return v1 - v2; },
		sol::meta_function::to_string, [](const glm::vec4& v1) -> std::string { return glm::to_string(v1); }
	);
	lua.new_usertype<glm::mat4>("mat4",
		sol::constructors<glm::mat4()>(),
		"translate", sol::factories([](glm::vec3 translation) -> glm::mat4 { return glm::translate(glm::mat4(1.0), translation); }),
		"rotate", sol::factories([](float degrees, glm::vec3 axis) -> glm::mat4 { return glm::rotate(glm::mat4(1.0), glm::radians(degrees), axis); }),
		// TODO find better way to make setters/getters
		"get", [](const glm::mat4& m1, int idx1, int idx2) -> float { return m1[idx1][idx2]; },
		"set", [](glm::mat4& m1, int idx1, int idx2, float value) { m1[idx1][idx2] = value; },
		sol::meta_function::multiplication, [](const glm::mat4& m1, const glm::mat4& m2) -> glm::mat4 { return m1 * m2; },
		sol::meta_function::to_string, [](const glm::mat4& m1) -> std::string { return glm::to_string(m1); }
	);

	lua.new_usertype<Frustum>("frustum",
		sol::constructors<Frustum(glm::mat4)>(),
		"isBoxVisible", [](const Frustum& frustum, glm::vec3 minBounds, glm::vec3 maxBounds) -> bool { return frustum.IsBoxVisible(minBounds, maxBounds); }
	);
}
