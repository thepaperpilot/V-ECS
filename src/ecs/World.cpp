#include "World.h"

#include <hastyNoise/hastyNoise.h>

#include <glm\ext\matrix_clip_space.hpp>
#include <glm/gtx/string_cast.hpp>

#include <stb/stb_image.h>
// rectpack2d:
#include "finders_interface.h"

#include <imgui.h>
#include <examples\imgui_impl_glfw.h>
#include <misc/cpp/imgui_stdlib.h>

#include "../events/LuaEvents.h"
#include "../lib/FrustumCull.h"
#include "../engine/Debugger.h"

using namespace vecs;
using namespace rectpack2D;
using namespace ImGui;

// Simple container for storing pixel data and the name of the file it came from
struct SubTexture {
	stbi_uc* pixels;
	std::string filename;
};

struct LuaCallback {
	LuaCallback(sol::table self, sol::function callback) {
		this->self = self;
		this->callback = callback;
	}

	sol::table self;
	sol::function callback;
};

static int InputTextCallback(ImGuiInputTextCallbackData* data) {
	LuaCallback* cb = (LuaCallback*)data->UserData;
	auto result = cb->callback(cb->self, data);
	if (result.valid()) {
		if (result.get_type() == sol::type::string) {
			std::string str = result;
			data->DeleteChars(0, data->BufTextLen);
			data->InsertChars(0, str.c_str());
			data->CursorPos = data->BufTextLen;
		}
	} else {
		sol::error err = result;
		Debugger::addLog(DEBUG_LEVEL_ERROR, "[LUA] " + std::string(err.what()));
		return 0;
	}
	return 0;
}

uint32_t World::createEntities(uint32_t amount) {
	uint32_t firstEntity = nextEntity;
	nextEntity += amount;
	return firstEntity;
}

void World::deleteEntity(uint32_t entity) {
	for (auto archetype : archetypes) {
		if (archetype->hasEntity(entity)) {
			archetype->removeEntities({ entity });
			return;
		}			
	}
}

Archetype* World::getArchetype(std::unordered_set<std::string> componentTypes, sol::table sharedComponents) {
	auto itr = std::find_if(archetypes.begin(), archetypes.end(), [&componentTypes, &sharedComponents](Archetype* archetype) {
		// TODO I don't think this'll deeply compare if the shared component tables are the same
		return archetype->componentTypes == componentTypes && archetype->sharedComponents == sharedComponents;
	});

	if (itr == archetypes.end()) {
		// Create a new archetype
		Archetype* archetype = archetypes.emplace_back(new Archetype(this, componentTypes, sharedComponents));

		// Add it to any entity queries it matches
		for (auto query : queries) {
			if (archetype->checkQuery(query))
				query->matchingArchetypes.push_back(archetype);
		}

		return archetype;
	}

	return *itr;
}

Archetype* World::getArchetype(uint32_t entity) {
	for (auto archetype : archetypes) {
		if (archetype->hasEntity(entity)) {
			return archetype;
		}
	}
	return nullptr;
}

bool World::hasComponentType(uint32_t entity, std::string component_t) {
	Archetype* archetype = getArchetype(entity);
	return archetype->componentTypes.count(component_t);
}

void World::addQuery(EntityQuery* query) {
	queries.push_back(query);

	// Check what archetypes match this query
	for (auto archetype : archetypes) {
		if (archetype->checkQuery(query))
			query->matchingArchetypes.push_back(archetype);
	}
}

void World::update(double deltaTime) {
	this->deltaTime = deltaTime;

	dependencyGraph.execute();
}

void World::windowRefresh(bool numImagesChanged, int imageCount) {
	dependencyGraph.windowRefresh(numImagesChanged, imageCount);
}

void World::cleanup() {
	// Destroy our systems and sub-renderers
	dependencyGraph.cleanup();

	// Destroy any buffers created by this world
	for (auto buffer : buffers)
		buffer.cleanup();
}

void World::setupState(Engine* engine) {
	lua.open_libraries(sol::lib::base, sol::lib::package, sol::lib::math, sol::lib::string, sol::lib::table);

	// Utility functions
	lua["getResources"] = [](std::string path, std::string extension) -> sol::as_table_t<std::vector<std::string>> {
		std::vector<std::string> resources;
		auto filepath = std::filesystem::path("./resources/" + path).make_preferred();
		for (const auto& entry : std::filesystem::directory_iterator(filepath))
			if (!entry.is_directory() && entry.path().extension() == extension)
				resources.push_back(entry.path().string());
		return resources;
	};
	lua["loadWorld"] = [engine](std::string filename) { engine->setWorld(filename); };

	lua.new_enum("sizes",
		"Float", sizeof(float),
		"Vec2", sizeof(glm::vec2),
		"Vec3", sizeof(glm::vec3),
		"Vec4", sizeof(glm::vec4),
		"Mat4", sizeof(glm::mat4)
	);

	// time namespace
	lua["time"] = lua.create_table_with(
		"getDeltaTime", [this]() -> float { return deltaTime; }
	);

	lua.new_enum("debugLevels",
		"Error", DEBUG_LEVEL_ERROR,
		"Warn", DEBUG_LEVEL_WARN,
		"Info", DEBUG_LEVEL_INFO,
		"Verbose", DEBUG_LEVEL_VERBOSE
	);
	lua.new_enum("shaderStages",
		"Vertex", VK_SHADER_STAGE_VERTEX_BIT,
		"Fragment", VK_SHADER_STAGE_FRAGMENT_BIT
	);
	lua.new_enum("vertexComponents",
		"Position", VERTEX_COMPONENT_POSITION,
		"Normal", VERTEX_COMPONENT_NORMAL,
		"UV", VERTEX_COMPONENT_UV,
		"MaterialIndex", VERTEX_COMPONENT_MATERIAL_INDEX,
		// These are used for custom vertex components that don't get set automatically by models
		// Intended for renderers that create their own vertex+index buffers
		"R32", VERTEX_COMPONENT_R32,
		"R32G32", VERTEX_COMPONENT_R32G32,
		"R32G32B32", VERTEX_COMPONENT_R32G32B32,
		"R32G32B32A32", VERTEX_COMPONENT_R32G32B32A32,
		"R8_UNORM", VERTEX_COMPONENT_R8_UNORM,
		"R8G8_UNORM", VERTEX_COMPONENT_R8G8_UNORM,
		"R8G8B8_UNORM", VERTEX_COMPONENT_R8G8B8_UNORM,
		"R8G8B8A8_UNORM", VERTEX_COMPONENT_R8G8B8A8_UNORM,
		"R32_UINT", VERTEX_COMPONENT_R32_UINT,
		"R32G32_UINT", VERTEX_COMPONENT_R32G32_UINT,
		"R32G32B32_UINT", VERTEX_COMPONENT_R32G32B32_UINT,
		"R32G32B32A32_UINT", VERTEX_COMPONENT_R32G32B32A32_UINT,
		"R32_SINT", VERTEX_COMPONENT_R32_SINT,
		"R32G32_SINT", VERTEX_COMPONENT_R32G32_SINT,
		"R32G32B32_SINT", VERTEX_COMPONENT_R32G32B32_SINT,
		"R32G32B32A32_SINT", VERTEX_COMPONENT_R32G32B32A32_SINT
	);
	lua.new_enum("materialComponents",
		"Diffuse", MATERIAL_COMPONENT_DIFFUSE
	);
	lua.new_enum("cullModes",
		"Back", VK_CULL_MODE_BACK_BIT,
		"Front", VK_CULL_MODE_FRONT_BIT,
		"None", VK_CULL_MODE_NONE
	);
	lua.new_enum("noiseType",
		"Cellular", HastyNoise::NoiseType::Cellular,
		"Cubic", HastyNoise::NoiseType::Cubic,
		"CubicFractal", HastyNoise::NoiseType::CubicFractal,
		"Perlin", HastyNoise::NoiseType::Perlin,
		"PerlinFractal", HastyNoise::NoiseType::PerlinFractal,
		"Simplex", HastyNoise::NoiseType::Simplex,
		"SimplexFractal", HastyNoise::NoiseType::SimplexFractal,
		"Value", HastyNoise::NoiseType::Value,
		"ValueFractal", HastyNoise::NoiseType::ValueFractal,
		"WhiteNoise", HastyNoise::NoiseType::WhiteNoise
	);
	lua.new_enum("cellularReturnType",
		"Distance2", HastyNoise::CellularReturnType::Distance2,
		"Distance2Add", HastyNoise::CellularReturnType::Distance2Add,
		"Distance2Cave", HastyNoise::CellularReturnType::Distance2Cave,
		"Distance2Div", HastyNoise::CellularReturnType::Distance2Div,
		"Distance2Mul", HastyNoise::CellularReturnType::Distance2Mul,
		"Distance2Sub", HastyNoise::CellularReturnType::Distance2Sub,
		"NoiseLookup", HastyNoise::CellularReturnType::NoiseLookup,
		"Value", HastyNoise::CellularReturnType::Value,
		"ValueDistance2", HastyNoise::CellularReturnType::ValueDistance2
	);
	lua.new_enum("perturbType",
		"Gradient", HastyNoise::PerturbType::Gradient,
		"GradientFractal", HastyNoise::PerturbType::GradientFractal,
		"GradientNormalise", HastyNoise::PerturbType::Gradient_Normalise,
		"GradientFractalNormalise", HastyNoise::PerturbType::GradientFractal_Normalise
	);
	lua.new_enum("bufferUsage",
		"VertexBuffer", VK_BUFFER_USAGE_VERTEX_BUFFER_BIT,
		"IndexBuffer", VK_BUFFER_USAGE_INDEX_BUFFER_BIT
	);
	lua.new_enum("windowFlags",
		"AlwaysAutoResize", ImGuiWindowFlags_AlwaysAutoResize,
		"AlwaysHorizontalScrollbar", ImGuiWindowFlags_AlwaysHorizontalScrollbar,
		"AlwaysUseWindowPadding", ImGuiWindowFlags_AlwaysUseWindowPadding,
		"AlwaysVerticalScrollbar", ImGuiWindowFlags_AlwaysVerticalScrollbar,
		"ChildMenu", ImGuiWindowFlags_ChildMenu,
		"ChildWindow", ImGuiWindowFlags_ChildWindow,
		"HorizontalScrollbar", ImGuiWindowFlags_HorizontalScrollbar,
		"MenuBar", ImGuiWindowFlags_MenuBar,
		"Modal", ImGuiWindowFlags_Modal,
		"NavFlattened", ImGuiWindowFlags_NavFlattened,
		"NoBackground", ImGuiWindowFlags_NoBackground,
		"NoBringToFrontOnFocus", ImGuiWindowFlags_NoBringToFrontOnFocus,
		"NoCollapse", ImGuiWindowFlags_NoCollapse,
		"NoDecoration", ImGuiWindowFlags_NoDecoration,
		"NoFocusOnAppearing", ImGuiWindowFlags_NoFocusOnAppearing,
		"NoInputs", ImGuiWindowFlags_NoInputs,
		"NoMouseInputs", ImGuiWindowFlags_NoMouseInputs,
		"NoMove", ImGuiWindowFlags_NoMove,
		"NoNav", ImGuiWindowFlags_NoNav,
		"NoNavFocus", ImGuiWindowFlags_NoNavFocus,
		"NoNavInputs", ImGuiWindowFlags_NoNavInputs,
		"NoResize", ImGuiWindowFlags_NoResize,
		"NoSavedSettings", ImGuiWindowFlags_NoSavedSettings,
		"NoScrollbar", ImGuiWindowFlags_NoScrollbar,
		"NoScrollWithMouse", ImGuiWindowFlags_NoScrollWithMouse,
		"NoTitleBar", ImGuiWindowFlags_NoTitleBar,
		"Popup", ImGuiWindowFlags_Popup,
		"Tooltip", ImGuiWindowFlags_Tooltip,
		"UnsavedDocument", ImGuiWindowFlags_UnsavedDocument
	);
	lua.new_enum("inputTextFlags",
		"AllowTabInput", ImGuiInputTextFlags_AllowTabInput,
		"AlwaysInsertMode", ImGuiInputTextFlags_AlwaysInsertMode,
		"AutoSelectAll", ImGuiInputTextFlags_AutoSelectAll,
		"CallbackAlways", ImGuiInputTextFlags_CallbackAlways,
		"CallbackCharFilter", ImGuiInputTextFlags_CallbackCharFilter,
		"CallbackCompletion", ImGuiInputTextFlags_CallbackCompletion,
		"CallbackHistory", ImGuiInputTextFlags_CallbackHistory,
		"CallbackResize", ImGuiInputTextFlags_CallbackResize,
		"CharsDecimal", ImGuiInputTextFlags_CharsDecimal,
		"CharsHexadecimal", ImGuiInputTextFlags_CharsHexadecimal,
		"CharsNoBlank", ImGuiInputTextFlags_CharsNoBlank,
		"CharsScientific", ImGuiInputTextFlags_CharsScientific,
		"CharsUppercase", ImGuiInputTextFlags_CharsUppercase,
		"CtrlEnterForNewLine", ImGuiInputTextFlags_CtrlEnterForNewLine,
		"EnterReturnsTrue", ImGuiInputTextFlags_EnterReturnsTrue,
		"Multiline", ImGuiInputTextFlags_Multiline,
		"NoHorizontalScroll", ImGuiInputTextFlags_NoHorizontalScroll,
		"NoMarkEdited", ImGuiInputTextFlags_NoMarkEdited,
		"NoUndoRedo", ImGuiInputTextFlags_NoUndoRedo,
		"Password", ImGuiInputTextFlags_Password,
		"ReadOnly", ImGuiInputTextFlags_ReadOnly
	);
	lua.new_enum("styleColors", 
		"Border", ImGuiCol_Border,
		"BorderShadow", ImGuiCol_BorderShadow,
		"Button", ImGuiCol_Button,
		"ButtonActive", ImGuiCol_ButtonActive,
		"ButtonHovered", ImGuiCol_ButtonHovered,
		"CheckMark", ImGuiCol_CheckMark,
		"ChildBg", ImGuiCol_ChildBg,
		"DragDropTarget", ImGuiCol_DragDropTarget,
		"FrameBg", ImGuiCol_FrameBg,
		"FrameBgActive", ImGuiCol_FrameBgActive,
		"FrameBgHovered", ImGuiCol_FrameBgHovered,
		"Header", ImGuiCol_Header,
		"HeaderActive", ImGuiCol_HeaderActive,
		"HeaderHovered", ImGuiCol_HeaderHovered,
		"MenuBarBg", ImGuiCol_MenuBarBg,
		"ModalWindowDarkening", ImGuiCol_ModalWindowDarkening,
		"ModalWindowDimBg", ImGuiCol_ModalWindowDimBg,
		"NavHighlight", ImGuiCol_NavHighlight,
		"NavWindowingDimBg", ImGuiCol_NavWindowingDimBg,
		"NavWindowingHighlight", ImGuiCol_NavWindowingHighlight,
		"PlotHistogram", ImGuiCol_PlotHistogram,
		"PlotHistogramHovered", ImGuiCol_PlotHistogramHovered,
		"PlotLines", ImGuiCol_PlotLines,
		"PlotLinesHovered", ImGuiCol_PlotLinesHovered,
		"PopupBg", ImGuiCol_PopupBg,
		"ResizeGrip", ImGuiCol_ResizeGrip,
		"ResizeGripActive", ImGuiCol_ResizeGripActive,
		"ResizeGripHovered", ImGuiCol_ResizeGripHovered,
		"ScrollbarBg", ImGuiCol_ScrollbarBg,
		"ScrollbarGrab", ImGuiCol_ScrollbarGrab,
		"ScrollbarGrabActive", ImGuiCol_ScrollbarGrabActive,
		"ScrollbarGrabHovered", ImGuiCol_ScrollbarGrabHovered,
		"Separator", ImGuiCol_Separator,
		"SeparatorActive", ImGuiCol_SeparatorActive,
		"SeparatorHovered", ImGuiCol_SeparatorHovered,
		"SliderGrab", ImGuiCol_SliderGrab,
		"SliderGrabActive", ImGuiCol_SliderGrabActive,
		"Tab", ImGuiCol_Tab,
		"TabActive", ImGuiCol_TabActive,
		"TabHovered", ImGuiCol_TabHovered,
		"TabUnfocused", ImGuiCol_TabUnfocused,
		"TabUnfocusedActive", ImGuiCol_TabUnfocusedActive,
		"Text", ImGuiCol_Text,
		"TextDisabled", ImGuiCol_TextDisabled,
		"TextSelectedBg", ImGuiCol_TextSelectedBg,
		"TitleBg", ImGuiCol_TitleBg,
		"TitleBgActive", ImGuiCol_TitleBgActive,
		"TitleBgCollapsed", ImGuiCol_TitleBgCollapsed,
		"WindowBg", ImGuiCol_WindowBg
	);
	lua.new_enum("styleVars",
		"Alpha", ImGuiStyleVar_Alpha,
		"ButtonTextAlign", ImGuiStyleVar_ButtonTextAlign,
		"ChildBorderSize", ImGuiStyleVar_ChildBorderSize,
		"ChildRounding", ImGuiStyleVar_ChildRounding,
		"FrameBorderSize", ImGuiStyleVar_FrameBorderSize,
		"FramePadding", ImGuiStyleVar_FramePadding,
		"FrameRounding", ImGuiStyleVar_FrameRounding,
		"GrabMinSize", ImGuiStyleVar_GrabMinSize,
		"GrabRounding", ImGuiStyleVar_GrabRounding,
		"IndentSpacing", ImGuiStyleVar_IndentSpacing,
		"ItemInnerSpacing", ImGuiStyleVar_ItemInnerSpacing,
		"ItemSpacing", ImGuiStyleVar_ItemSpacing,
		"PopupBorderSize", ImGuiStyleVar_PopupBorderSize,
		"PopupRounding", ImGuiStyleVar_PopupRounding,
		"ScrollbarRounding", ImGuiStyleVar_ScrollbarRounding,
		"ScrollbarSize", ImGuiStyleVar_ScrollbarSize,
		"SelectableTextAlign", ImGuiStyleVar_SelectableTextAlign,
		"TabRounding", ImGuiStyleVar_TabRounding,
		"WindowBorderSize", ImGuiStyleVar_WindowBorderSize,
		"WindowMinSize", ImGuiStyleVar_WindowMinSize,
		"WindowPadding", ImGuiStyleVar_WindowPadding,
		"WindowRounding", ImGuiStyleVar_WindowRounding,
		"WindowTitleAlign", ImGuiStyleVar_WindowTitleAlign
	);
	lua.new_enum("keys",
		"Space", GLFW_KEY_SPACE,
		"Apostrophe", GLFW_KEY_APOSTROPHE,
		"Comma", GLFW_KEY_COMMA,
		"Minus", GLFW_KEY_MINUS,
		"Period", GLFW_KEY_PERIOD,
		"Slash", GLFW_KEY_SLASH,
		"0", GLFW_KEY_0,
		"1", GLFW_KEY_1,
		"2", GLFW_KEY_2,
		"3", GLFW_KEY_3,
		"4", GLFW_KEY_4,
		"5", GLFW_KEY_5,
		"6", GLFW_KEY_6,
		"7", GLFW_KEY_7,
		"8", GLFW_KEY_8,
		"9", GLFW_KEY_9,
		"W", GLFW_KEY_W,
		"Semicolon", GLFW_KEY_SEMICOLON,
		"Equal", GLFW_KEY_EQUAL,
		"A", GLFW_KEY_A,
		"B", GLFW_KEY_B,
		"C", GLFW_KEY_C,
		"D", GLFW_KEY_D,
		"E", GLFW_KEY_E,
		"F", GLFW_KEY_F,
		"G", GLFW_KEY_G,
		"H", GLFW_KEY_H,
		"I", GLFW_KEY_I,
		"J", GLFW_KEY_J,
		"K", GLFW_KEY_K,
		"L", GLFW_KEY_L,
		"M", GLFW_KEY_M,
		"N", GLFW_KEY_N,
		"O", GLFW_KEY_O,
		"P", GLFW_KEY_P,
		"Q", GLFW_KEY_Q,
		"R", GLFW_KEY_R,
		"S", GLFW_KEY_S,
		"T", GLFW_KEY_T,
		"U", GLFW_KEY_U,
		"V", GLFW_KEY_V,
		"W", GLFW_KEY_W,
		"X", GLFW_KEY_X,
		"Y", GLFW_KEY_Y,
		"Z", GLFW_KEY_Z,
		// We start skipping once we get to the function keys,
		// only binding the ones we think we'll need
		"LeftBracket", GLFW_KEY_LEFT_BRACKET,
		"Backslash", GLFW_KEY_BACKSLASH,
		"RightBracket", GLFW_KEY_RIGHT_BRACKET,
		"Grave", GLFW_KEY_GRAVE_ACCENT,
		"Escape", GLFW_KEY_ESCAPE,
		"Enter", GLFW_KEY_ENTER,
		"Tab", GLFW_KEY_TAB,
		"Backspace", GLFW_KEY_BACKSPACE,
		"Insert", GLFW_KEY_INSERT,
		"Delete", GLFW_KEY_DELETE,
		"Right", GLFW_KEY_RIGHT,
		"Left", GLFW_KEY_LEFT,
		"Down", GLFW_KEY_DOWN,
		"Up", GLFW_KEY_UP,
		"PageUp", GLFW_KEY_PAGE_UP,
		"PageDown", GLFW_KEY_PAGE_DOWN,
		"Home", GLFW_KEY_HOME,
		"End", GLFW_KEY_END,
		"LeftShift", GLFW_KEY_LEFT_SHIFT,
		"LeftControl", GLFW_KEY_LEFT_CONTROL,
		"LeftAlt", GLFW_KEY_LEFT_ALT,
		"LeftSuper", GLFW_KEY_LEFT_SUPER,
		"RightShift", GLFW_KEY_RIGHT_SHIFT,
		"RightControl", GLFW_KEY_RIGHT_CONTROL,
		"RightAlt", GLFW_KEY_RIGHT_ALT,
		"RightSuper", GLFW_KEY_RIGHT_SUPER
	);

	// glfw namespace
	lua["glfw"] = lua.create_table_with(
		"hideCursor", [window = engine->window]() { glfwSetInputMode(window, GLFW_CURSOR, GLFW_CURSOR_DISABLED); },
		"showCursor", [window = engine->window]() { glfwSetInputMode(window, GLFW_CURSOR, GLFW_CURSOR_NORMAL); },
		"isCursorVisible", [window = engine->window]() -> bool { return glfwGetInputMode(window, GLFW_CURSOR) == GLFW_CURSOR_NORMAL; },
		"cursorPos", [window = engine->window]() -> std::tuple<double, double> {
			double xpos, ypos;
			glfwGetCursorPos(window, &xpos, &ypos);
			return std::make_tuple(xpos, ypos);
		},
		"isKeyPressed", [window = engine->window](int key) -> bool { return glfwGetKey(window, key) == GLFW_PRESS; },
		"windowSize", [window = engine->window]() -> std::tuple<int, int> {
			int width, height;
			glfwGetWindowSize(window, &width, &height);
			return std::make_tuple(width, height);
		}
	);

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
		"length", [](const glm::vec3& v1) -> float {
			return glm::length(v1);
		},
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
		// TODO find better way to make setters/getters
		"get", [](const glm::mat4& m1, int idx1, int idx2) -> float { return m1[idx1][idx2]; },
		"set", [](glm::mat4& m1, int idx1, int idx2, float value) { m1[idx1][idx2] = value; },
		sol::meta_function::multiplication, [](const glm::mat4& m1, const glm::mat4& m2) -> glm::mat4 {
			return m1 * m2;
		},
		sol::meta_function::to_string, [](const glm::mat4& m1) -> std::string { return glm::to_string(m1); }
	);

	lua.new_usertype<Frustum>("frustum",
		sol::constructors<Frustum(glm::mat4)>(),
		"isBoxVisible", [](const Frustum& frustum, glm::vec3 minBounds, glm::vec3 maxBounds) -> bool { return frustum.IsBoxVisible(minBounds, maxBounds); }
	);

	// ecs
	lua.new_usertype<Archetype>("archetype",
		"new", sol::factories(
			[this](std::unordered_set<std::string> required) -> Archetype* { return this->getArchetype(required); },
			[this](std::unordered_set<std::string> required, sol::table sharedComponents) -> Archetype* { return this->getArchetype(required, sharedComponents); }
		),
		"isEmpty", [](const Archetype& archetype) -> bool { return archetype.componentTypes.size(); },
		"getComponents", [](Archetype& archetype, std::string componentType) -> sol::table {
			auto t = archetype.getComponentList(componentType);
			t.size();
			return archetype.getComponentList(componentType);
		},
		"getSharedComponent", [](Archetype& archetype, std::string component_t) -> sol::table { return archetype.getSharedComponent(component_t); },
		"createEntity", [](Archetype& archetype) -> std::pair<uint32_t, size_t> { return archetype.createEntities(1); },
		"createEntities", &Archetype::createEntities
	);
	lua.new_usertype<EntityQuery>("query",
		"new", sol::factories(
			[this](std::vector<std::string> required) -> EntityQuery* {
				EntityQuery* query = new EntityQuery();
				query->filter.with(required);
				this->addQuery(query);
				return query;
			}, [this](std::vector<std::string> required, std::vector<std::string> disallowed) -> EntityQuery* {
				EntityQuery* query = new EntityQuery();
				query->filter.with(required);
				query->filter.without(disallowed);
				this->addQuery(query);
				return query;
			}
		),
		"getArchetypes", [](const EntityQuery& query) -> sol::as_table_t<std::vector<Archetype*>> { return query.matchingArchetypes; }
	);

	// events
	lua.new_usertype<LuaEvent>("event",
		sol::constructors<LuaEvent()>(),
		"add", [](LuaEvent* event, sol::table self, sol::function cb) {
			event->selves.emplace_back(self);
			event->listeners.emplace_back(cb);
		},
		"fire", &LuaEvent::fire
	);
	// Create glfw events
	lua["glfw"]["events"] = lua.create_table_with(
		"WindowRefresh", &(new WindowRefreshLuaEvent(&lua, this))->event,
		"WindowResize", &(new WindowResizeLuaEvent(&lua, this))->event,
		"MouseMove", &(new MouseMoveLuaEvent(&lua, this))->event,
		"LeftMousePress", &(new LeftMousePressLuaEvent(&lua, this))->event,
		"LeftMouseRelease", &(new LeftMouseReleaseLuaEvent(&lua, this))->event,
		"RightMousePress", &(new RightMousePressLuaEvent(&lua, this))->event,
		"RightMouseRelease", &(new RightMouseReleaseLuaEvent(&lua, this))->event,
		"VerticalScroll", &(new VerticalScrollLuaEvent(&lua, this))->event,
		"HorizontalScroll", &(new HorizontalScrollLuaEvent(&lua, this))->event,
		"KeyPress", &(new KeyPressLuaEvent(&lua, this))->event,
		"KeyRelease", &(new KeyReleaseLuaEvent(&lua, this))->event
	);

	// noise
	lua.new_usertype<HastyNoise::NoiseSIMD>("noise",
		"new", sol::factories(
			[engine](HastyNoise::NoiseType noiseType, int seed, float frequency) -> HastyNoise::NoiseSIMD* {
				HastyNoise::NoiseSIMD* noise = HastyNoise::details::CreateNoise(seed, engine->fastestSimd);
				noise->SetNoiseType(noiseType);
				noise->SetFrequency(frequency);
				return noise;
			}
		),
		"getNoiseSet", [](HastyNoise::NoiseSIMD& noise, int chunkX, int chunkY, int chunkZ, int chunkSize) -> sol::as_table_t<std::vector<float>> {
			auto floatBuffer = noise.GetNoiseSet(chunkX * chunkSize, chunkY * chunkSize, chunkZ * chunkSize, chunkSize, chunkSize, chunkSize);
			return std::vector<float>(floatBuffer.get(), floatBuffer.get() + chunkSize * chunkSize * chunkSize);
		},
		"setCellularReturnType", &HastyNoise::NoiseSIMD::SetCellularReturnType,
		"setCellularJitter", &HastyNoise::NoiseSIMD::SetCellularJitter,
		"setPerturbType", &HastyNoise::NoiseSIMD::SetPerturbType,
		"setPerturbAmp", &HastyNoise::NoiseSIMD::SetPerturbAmp,
		"setPerturbFrequency", &HastyNoise::NoiseSIMD::SetPerturbFrequency,
		"setPerturbFractalOctaves", &HastyNoise::NoiseSIMD::SetPerturbFractalOctaves,
		"setPerturbFractalLacunarity", &HastyNoise::NoiseSIMD::SetPerturbFractalLacunarity,
		"setPerturbFractalGain", &HastyNoise::NoiseSIMD::SetPerturbFractalGain
	);

	// rendering
	lua.new_usertype<SubRenderer>("renderer",
		"new", sol::no_constructor,
		"addTexture", [this](SubRenderer renderer, Texture* texture) {
			VkDescriptorSet descriptorSet;

			VkDescriptorSetAllocateInfo allocateInfo = {};
			allocateInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_ALLOCATE_INFO;
			allocateInfo.descriptorPool = renderer.imguiDescriptorPool;
			allocateInfo.descriptorSetCount = 1;
			allocateInfo.pSetLayouts = &renderer.descriptorSetLayout;

			VK_CHECK_RESULT(vkAllocateDescriptorSets(device->logical, &allocateInfo, &descriptorSet));

			VkDescriptorImageInfo descriptorImage[1] = { texture->descriptor };
			VkWriteDescriptorSet writeDescriptor[1] = {};
			writeDescriptor[0].sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
			writeDescriptor[0].dstSet = descriptorSet;
			writeDescriptor[0].dstBinding = 1;
			writeDescriptor[0].descriptorCount = 1;
			writeDescriptor[0].descriptorType = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
			writeDescriptor[0].pImageInfo = descriptorImage;

			vkUpdateDescriptorSets(device->logical, 1, writeDescriptor, 0, nullptr);
			
			texture->imguiTexId = (ImTextureID)descriptorSet;
		},
		"pushConstantMat4", [](SubRenderer renderer, VkShaderStageFlags shaderStage, int offset, glm::mat4 constant) {
			vkCmdPushConstants(renderer.activeCommandBuffer, renderer.pipelineLayout, shaderStage, offset, sizeof(glm::mat4), &constant);
		},
		"pushConstantVec3", [](SubRenderer renderer, VkShaderStageFlags shaderStage, int offset, glm::vec3 constant) {
			vkCmdPushConstants(renderer.activeCommandBuffer, renderer.pipelineLayout, shaderStage, offset, sizeof(glm::vec3), &constant);
		},
		"draw", [](SubRenderer renderer, Model& model) { model.draw(renderer.activeCommandBuffer, renderer.pipelineLayout); },
		"drawVertices", [](SubRenderer renderer, Buffer* vertexBuffer, Buffer* indexBuffer, int indexCount) {
			VkBuffer vertexBuffers[] = { vertexBuffer->buffer };
			VkDeviceSize offsets[] = { 0 };
			vkCmdBindVertexBuffers(renderer.activeCommandBuffer, 0, 1, vertexBuffers, offsets);
			vkCmdBindIndexBuffer(renderer.activeCommandBuffer, indexBuffer->buffer, 0, VK_INDEX_TYPE_UINT32);
			vkCmdDrawIndexed(renderer.activeCommandBuffer, indexCount, 1, 0, 0, 0);
		}
	);
	lua.new_usertype<Model>("model",
		"new", sol::constructors<Model(SubRenderer*, const char*), Model(SubRenderer*, const char*, VkShaderStageFlagBits, std::vector<MaterialComponent>)>(),
		"minBounds", &Model::minBounds,
		"maxBounds", &Model::maxBounds
	);
	lua.new_usertype<Texture>("texture",
		sol::constructors<Texture(SubRenderer*, const char*)>(),
		"createStitched", [this, engine](SubRenderer* subrenderer, std::vector<std::string> images) -> std::tuple<Texture*, sol::table> {
			using spaces_type = empty_spaces<false, default_empty_spaces>;
			using rect_type = output_rect_t<spaces_type>;

			uint8_t texIdx = 0;
			sol::table map = lua.create_table();
			std::vector<SubTexture> subtextures;
			std::vector<rect_type> rects;

			// Read pixels for each sub-texture and add their size to our rects list
			subtextures.resize(images.size());
			rects.resize(images.size());
			for (auto image : images) {
				int texChannels, texWidth, texHeight;
				subtextures[texIdx].pixels = stbi_load(image.c_str(), &texWidth, &texHeight, &texChannels, STBI_rgb_alpha);
				subtextures[texIdx].filename = std::filesystem::path(image).filename().string();
				// We add a pixel to the height and left to help prevent bleeding
				rects[texIdx] = { 0, 0, texWidth, texHeight };
				texIdx++;
			}

			// Find optimal packing for our textures
			const auto texSize = find_best_packing<spaces_type>(
				rects,
				make_finder_input(
					// TODO how to handle more textures than can fit in here
					// (and can the GPU handle that much?)
					32768,
					1,
					[](rect_type&) {
						return callback_result::CONTINUE_PACKING;
					},
					[](rect_type&) {
						// TODO what to do if we can't fit all the textures?
						return callback_result::ABORT_PACKING;
					},
						flipping_option::DISABLED
				)
			);

			// Calculate UVs for each texture and store them in our map,
			// and store their pixels in our stitched texture pixels array
			unsigned char* pixels = new unsigned char[texSize.w * texSize.h * 4];
			for (int i = 0; i < subtextures.size(); i++) {
				auto rect = rects[i];
				map[subtextures[i].filename] = lua.create_table_with(
					"p", rect.x / (float)texSize.w,
					"q", rect.y / (float)texSize.h,
					"s", (rect.x + rect.w) / (float)texSize.w,
					"t", (rect.y + rect.h) / (float)texSize.h,
					"width", rect.w,
					"height", rect.h
				);
				for (int y = 0; y < rect.h; y++) {
					int mainOffset = (rect.y + y) * (texSize.w * 4) + rect.x * 4;
					int subOffset = y * (rect.w * 4);
					for (int x = 0; x < rect.w * 4; x++) {
						pixels[mainOffset + x] = subtextures[i].pixels[subOffset + x];
					}
				}
			}

			// Create our texture and release the stbi pixel data
			return std::make_tuple(new Texture(subrenderer, pixels, texSize.w, texSize.h), map);
		}
	);
	lua.new_usertype<Buffer>("buffer",
		"new", sol::factories(
			[this](VkBufferUsageFlags usageFlags, VkDeviceSize size) -> Buffer {
				Buffer buffer = device->createBuffer(size, usageFlags, VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT);
				buffers.emplace_back(buffer);
				return buffer;
			}
		),
		"setDataInts", [](Buffer buffer, std::vector<int> data) {
			buffer.copyTo(data.data(), data.size() * sizeof(int));
		},
		"setDataFloats", [](Buffer buffer, std::vector<float> data) {
			buffer.copyTo(data.data(), data.size() * sizeof(float));
		}
	);

	// imgui
	lua["ig"] = lua.create_table_with(
		"preInit", [this](SubRenderer* subrenderer) {
			// We'll be using multiple textures but only one per draw call, so we tell our subrenderer
			// about a dummy texture in preinit(),
			// and actually create the font texture and prepare for user textures in init()
			unsigned char pixels[4] = { 0 };
			new Texture(subrenderer, pixels, 1, 1);

			// Not all of these are in use, but the pool is cheap and its what imgui
			// does in its vulkan+glfw example:
			// https://github.com/ocornut/imgui/blob/master/examples/example_glfw_vulkan/main.cpp
			VkDescriptorPoolSize pool_sizes[] =
			{
				{ VK_DESCRIPTOR_TYPE_SAMPLER, 1000 },
				{ VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, 1000 },
				{ VK_DESCRIPTOR_TYPE_SAMPLED_IMAGE, 1000 },
				{ VK_DESCRIPTOR_TYPE_STORAGE_IMAGE, 1000 },
				{ VK_DESCRIPTOR_TYPE_UNIFORM_TEXEL_BUFFER, 1000 },
				{ VK_DESCRIPTOR_TYPE_STORAGE_TEXEL_BUFFER, 1000 },
				{ VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER, 1000 },
				{ VK_DESCRIPTOR_TYPE_STORAGE_BUFFER, 1000 },
				{ VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER_DYNAMIC, 1000 },
				{ VK_DESCRIPTOR_TYPE_STORAGE_BUFFER_DYNAMIC, 1000 },
				{ VK_DESCRIPTOR_TYPE_INPUT_ATTACHMENT, 1000 }
			};
			VkDescriptorPoolCreateInfo pool_info = {};
			pool_info.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_POOL_CREATE_INFO;
			pool_info.flags = VK_DESCRIPTOR_POOL_CREATE_FREE_DESCRIPTOR_SET_BIT;
			pool_info.maxSets = 1000 * IM_ARRAYSIZE(pool_sizes);
			pool_info.poolSizeCount = (uint32_t)IM_ARRAYSIZE(pool_sizes);
			pool_info.pPoolSizes = pool_sizes;
			VK_CHECK_RESULT(vkCreateDescriptorPool(device->logical, &pool_info, nullptr, &subrenderer->imguiDescriptorPool));
		},
		"init", [this](SubRenderer* subrenderer) {
			// Create our font texture from ImGUI's pixel data
			ImGuiIO& io = ImGui::GetIO();
			unsigned char* pixels;
			int width, height;
			io.Fonts->GetTexDataAsRGBA32(&pixels, &width, &height);
			Texture* fontTexture = new Texture(subrenderer, pixels, width, height);

			// Copied from renderer's addTexture function
			VkDescriptorSet descriptorSet;

			VkDescriptorSetAllocateInfo allocateInfo = {};
			allocateInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_ALLOCATE_INFO;
			allocateInfo.descriptorPool = subrenderer->imguiDescriptorPool;
			allocateInfo.descriptorSetCount = 1;
			allocateInfo.pSetLayouts = &subrenderer->descriptorSetLayout;

			VK_CHECK_RESULT(vkAllocateDescriptorSets(device->logical, &allocateInfo, &descriptorSet));

			VkDescriptorImageInfo descriptorImage[1] = { fontTexture->descriptor };
			VkWriteDescriptorSet writeDescriptor[1] = {};
			writeDescriptor[0].sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
			writeDescriptor[0].dstSet = descriptorSet;
			writeDescriptor[0].dstBinding = 1;
			writeDescriptor[0].descriptorCount = 1;
			writeDescriptor[0].descriptorType = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
			writeDescriptor[0].pImageInfo = descriptorImage;

			vkUpdateDescriptorSets(device->logical, 1, writeDescriptor, 0, nullptr);

			io.Fonts->TexID = (ImTextureID)descriptorSet;
		},
		"newFrame", []() {
			ImGui_ImplGlfw_NewFrame();
			ImGui::NewFrame();
		},
		// TODO Is there a way to make this shorter/cleaner/somewhere else
		"render", [engine, device = this->device](SubRenderer* subrenderer) {
			ImGui::Render();

			auto draw_data = ImGui::GetDrawData();

			// Resize our vertex and index buffers if necessary
			size_t vertexSize = draw_data->TotalVtxCount;
			size_t indexSize = draw_data->TotalIdxCount;

			if (vertexSize == 0) return;

			if (engine->imguiVertexBufferSize < vertexSize) {
				size_t size = engine->imguiVertexBufferSize || 1;
				// Double size until its equal to or greater than minSize
				while (size < vertexSize)
					size <<= 1;
				// recreate our vertex buffer with the new size
				engine->imguiVertexBuffer.cleanup();
				engine->imguiVertexBuffer = device->createBuffer(
					size * sizeof(ImDrawVert),
					VK_BUFFER_USAGE_VERTEX_BUFFER_BIT,
					VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT);
				engine->imguiVertexBufferSize = size;
			}
			if (engine->imguiIndexBufferSize < indexSize) {
				size_t size = engine->imguiIndexBufferSize || 1;
				// Double size until its equal to or greater than minSize
				while (size < indexSize)
					size <<= 1;
				// recreate our index buffer with the new size
				engine->imguiIndexBuffer.cleanup();
				engine->imguiIndexBuffer = device->createBuffer(
					size * sizeof(ImDrawIdx),
					VK_BUFFER_USAGE_INDEX_BUFFER_BIT,
					VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT);
				engine->imguiIndexBufferSize = size;
			}

			// Fill our index and vertex buffers
			ImDrawVert* vtx_dst = (ImDrawVert*)engine->imguiVertexBuffer.map(vertexSize * sizeof(ImDrawVert), 0);
			ImDrawIdx* idx_dst = (ImDrawIdx*)engine->imguiIndexBuffer.map(indexSize * sizeof(ImDrawIdx), 0);
			for (int n = 0; n < draw_data->CmdListsCount; n++) {
				const ImDrawList* cmd_list = draw_data->CmdLists[n];
				engine->imguiVertexBuffer.copyTo(vtx_dst, cmd_list->VtxBuffer.Data, cmd_list->VtxBuffer.Size * sizeof(ImDrawVert));
				engine->imguiIndexBuffer.copyTo(idx_dst, cmd_list->IdxBuffer.Data, cmd_list->IdxBuffer.Size * sizeof(ImDrawIdx));
				vtx_dst += cmd_list->VtxBuffer.Size;
				idx_dst += cmd_list->IdxBuffer.Size;
			}
			// Flush our memory
			VkMappedMemoryRange range[2] = {};
			range[0].sType = VK_STRUCTURE_TYPE_MAPPED_MEMORY_RANGE;
			range[0].memory = engine->imguiVertexBuffer.memory;
			range[0].size = VK_WHOLE_SIZE;
			range[1].sType = VK_STRUCTURE_TYPE_MAPPED_MEMORY_RANGE;
			range[1].memory = engine->imguiIndexBuffer.memory;
			range[1].size = VK_WHOLE_SIZE;
			VK_CHECK_RESULT(vkFlushMappedMemoryRanges(device->logical, 2, range));
			engine->imguiVertexBuffer.unmap();
			engine->imguiIndexBuffer.unmap();

			// Submit our push constants
			float scale[2];
			scale[0] = 2.0f / draw_data->DisplaySize.x;
			scale[1] = 2.0f / draw_data->DisplaySize.y;
			float translate[2];
			translate[0] = -1.0f - draw_data->DisplayPos.x * scale[0];
			translate[1] = -1.0f - draw_data->DisplayPos.y * scale[1];

			vkCmdPushConstants(subrenderer->activeCommandBuffer, subrenderer->pipelineLayout, VK_SHADER_STAGE_VERTEX_BIT, sizeof(float) * 0, sizeof(float) * 2, scale);
			vkCmdPushConstants(subrenderer->activeCommandBuffer, subrenderer->pipelineLayout, VK_SHADER_STAGE_VERTEX_BIT, sizeof(float) * 2, sizeof(float) * 2, translate);

			// Bind our vertex and index buffers
			VkBuffer vertexBuffers[] = { engine->imguiVertexBuffer };
			VkDeviceSize offsets[] = { 0 };
			vkCmdBindVertexBuffers(subrenderer->activeCommandBuffer, 0, 1, vertexBuffers, offsets);
			vkCmdBindIndexBuffer(subrenderer->activeCommandBuffer, engine->imguiIndexBuffer, 0, sizeof(ImDrawIdx) == 2 ? VK_INDEX_TYPE_UINT16 : VK_INDEX_TYPE_UINT32);

			// Will project scissor/clipping rectangles into framebuffer space
			ImVec2 clip_off = draw_data->DisplayPos;         // (0,0) unless using multi-viewports
			ImVec2 clip_scale = draw_data->FramebufferScale; // (1,1) unless using retina display which are often (2,2)

			// Render command lists
			// (Because we merged all buffers into a single one, we maintain our own offset into them)
			int global_vtx_offset = 0;
			int global_idx_offset = 0;
			for (int n = 0; n < draw_data->CmdListsCount; n++) {
				const ImDrawList* cmd_list = draw_data->CmdLists[n];
				for (int cmd_i = 0; cmd_i < cmd_list->CmdBuffer.Size; cmd_i++) {
					const ImDrawCmd* pcmd = &cmd_list->CmdBuffer[cmd_i];
					if (pcmd->UserCallback != NULL) {
						// User callback, registered via ImDrawList::AddCallback()
						// (ImDrawCallback_ResetRenderState is a special callback value used by the user to request the renderer to reset render state.)
						if (pcmd->UserCallback == ImDrawCallback_ResetRenderState) {
							// TODO how to implement this?
						}
						else pcmd->UserCallback(cmd_list, pcmd);
					}
					else {
						// Project scissor/clipping rectangles into framebuffer space
						ImVec4 clip_rect;
						clip_rect.x = (pcmd->ClipRect.x - clip_off.x) * clip_scale.x;
						clip_rect.y = (pcmd->ClipRect.y - clip_off.y) * clip_scale.y;
						clip_rect.z = (pcmd->ClipRect.z - clip_off.x) * clip_scale.x;
						clip_rect.w = (pcmd->ClipRect.w - clip_off.y) * clip_scale.y;

						if (clip_rect.x < subrenderer->renderer->swapChainExtent.width && clip_rect.y < subrenderer->renderer->swapChainExtent.height &&
							clip_rect.z >= 0.0f && clip_rect.w >= 0.0f) {

							// Negative offsets are illegal for vkCmdSetScissor
							if (clip_rect.x < 0.0f)
								clip_rect.x = 0.0f;
							if (clip_rect.y < 0.0f)
								clip_rect.y = 0.0f;

							// Apply scissor/clipping rectangle
							VkRect2D scissor;
							scissor.offset.x = (int32_t)(clip_rect.x);
							scissor.offset.y = (int32_t)(clip_rect.y);
							scissor.extent.width = (uint32_t)(clip_rect.z - clip_rect.x);
							scissor.extent.height = (uint32_t)(clip_rect.w - clip_rect.y);
							vkCmdSetScissor(subrenderer->activeCommandBuffer, 0, 1, &scissor);

							// Bind descriptor set with font or user texture
							// TODO slight inefficiency when SubRenderer binds the font descriptor set
							VkDescriptorSet descSet[1] = { (VkDescriptorSet)pcmd->TextureId };
							vkCmdBindDescriptorSets(subrenderer->activeCommandBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS, subrenderer->pipelineLayout, 0, 1, descSet, 0, NULL);

							// Draw
							vkCmdDrawIndexed(subrenderer->activeCommandBuffer, pcmd->ElemCount, 1, pcmd->IdxOffset + global_idx_offset, pcmd->VtxOffset + global_vtx_offset, 0);
						}
					}
				}
				global_idx_offset += cmd_list->IdxBuffer.Size;
				global_vtx_offset += cmd_list->VtxBuffer.Size;
			}
		},
		"setNextWindowPos", [](int x, int y) { SetNextWindowPos(ImVec2(x, y)); },
		"setNextWindowSize", [](int width, int height) { SetNextWindowSize(ImVec2(width, height)); },
		"setWindowFontScale", &SetWindowFontScale,
		"beginWindow", [](std::string title, bool* p_open, std::vector<ImGuiWindowFlags> flags) {
			ImGuiWindowFlags windowFlags = 0;
			for (auto flag : flags)
				windowFlags |= flag;
			Begin(title.c_str(), p_open, windowFlags);
		},
		"endWindow", &End,
		"openPopup", &OpenPopup,
		"beginPopupModal", [](std::string title, bool* p_open, std::vector<ImGuiWindowFlags> flags) {
			ImGuiWindowFlags windowFlags = 0;
			for (auto flag : flags)
				windowFlags |= flag;
			BeginPopupModal(title.c_str(), p_open, windowFlags);
		},
		"endPopup", &EndPopup,
		"beginChild", [](std::string title, glm::vec2 size, bool border, std::vector<ImGuiWindowFlags> flags) {
			ImGuiWindowFlags windowFlags = 0;
			for (auto flag : flags)
				windowFlags |= flag;
			BeginChild(title.c_str(), ImVec2(size.x, size.y), border, windowFlags);
		},
		"endChild", &EndChild,
		"beginTooltip", []() { BeginTooltip(); },
		"endTooltip", []() { EndTooltip(); },
		"logToClipboard", []() { LogToClipboard(); },
		"logFinish", &LogFinish,
		"pushStyleColor", [](ImGuiCol idx, glm::vec4 color) { PushStyleColor(idx, ImVec4(color.r, color.g, color.b, color.a)); },
		"popStyleColor", [](int amount) { PopStyleColor(amount); },
		"pushStyleVar", sol::overload([](ImGuiStyleVar idx, float val) { PushStyleVar(idx, val); }, [](ImGuiStyleVar idx, float v1, float v2) { PushStyleVar(idx, ImVec2(v1, v2)); }),
		"popStyleVar", [](int amount) { PopStyleVar(amount); },
		"pushTextWrapPos", [](float pos) { PushTextWrapPos(pos); },
		"popTextWrapPos", []() { PopTextWrapPos(); },
		"setKeyboardFocusHere", sol::overload([]() { SetKeyboardFocusHere(); }, &SetKeyboardFocusHere),
		"getScrollY", &GetScrollX,
		"getScrollY", &GetScrollY,
		"setScrollHereX", sol::overload([]() { SetScrollHereX(); }, &SetScrollHereX),
		"setScrollHereY", sol::overload([]() { SetScrollHereY(); }, &SetScrollHereY),
		"getScrollMaxX", &GetScrollMaxX,
		"getScrollMaxY", &GetScrollMaxY,
		"calcTextSize", [](std::string text) -> std::tuple<float, float> {
			ImVec2 size = CalcTextSize(text.c_str());
			return std::make_tuple(size.x, size.y);
		},
		"image", [](Texture* texture, glm::vec2 size, glm::vec4 uv) {
			// Note we flip q and t - that's because vulkan flips the y component compared to opengl
			Image((ImTextureID)texture->imguiTexId, ImVec2(size.x, size.y), ImVec2(uv.p, uv.t), ImVec2(uv.s, uv.q));
		},
		"sameLine", []() { SameLine(); },
		"getCursorPos", []() -> glm::vec2 {
			auto pos = GetCursorPos();
			return glm::vec2(pos.x, pos.y);
		},
		"setCursorPos", [](float x, float y) { SetCursorPos(ImVec2(x, y)); },
		"text", [](std::string text) { Text(text.c_str()); },
		"textWrapped", [](std::string text) { TextWrapped(text.c_str()); },
		"textUnformatted", [](std::string text) { TextUnformatted(text.c_str()); },
		"button", sol::overload([](std::string text) -> bool { return Button(text.c_str()); }, [](std::string text, glm::vec2 size) -> bool { return Button(text.c_str(), ImVec2(size.x, size.y)); }),
		"checkbox", [](std::string label, bool value) -> bool {
			Checkbox(label.c_str(), &value);
			return value;
		},
		"progressBar", [](float percent) { ProgressBar(percent); },
		"isItemHovered", []() -> bool { return IsItemHovered(); },
		"isMouseClicked", []() -> bool { return IsMouseClicked(ImGuiMouseButton_Left); },
		"separator", &Separator,
		"spacing", &Spacing,
		"showDemoWindow", []() { ShowDemoWindow(); },
		"inputText", [](std::string label, std::string input, std::vector<ImGuiInputTextFlags> flags, sol::table self, sol::function callback) -> std::tuple<bool,std::string> {
			ImGuiInputTextFlags inputTextFlags = 0;
			for (auto flag : flags)
				inputTextFlags |= flag;
			bool result = InputText(label.c_str(), &input, inputTextFlags, InputTextCallback, new LuaCallback(self, callback));
			// Since we can't create a string pointer in our lua code, we return the updated string alongside the input text's return value
			return std::make_tuple(result, input);
		}
	);
	lua.new_usertype<ImGuiTextFilter>("textFilter",
		sol::constructors<ImGuiTextFilter()>(),
		"draw", [](ImGuiTextFilter* filter, std::string label) { filter->Draw(label.c_str()); },
		"check", [](ImGuiTextFilter* filter, std::string text) -> bool { return filter->PassFilter(text.c_str()); }
	);
	lua.new_usertype<ImGuiTextEditCallbackData>("textEditCallbackData",
		"new", sol::no_constructor,
		"eventFlag", &ImGuiTextEditCallbackData::EventFlag,
		"getKey", [](ImGuiTextEditCallbackData* data) -> int {
			// We map imgui keys to GLFW keys so we only need one keys enum
			switch (data->EventKey) {
			case ImGuiKey_Tab:
				return GLFW_KEY_TAB;
			case ImGuiKey_LeftArrow:
				return GLFW_KEY_LEFT;
			case ImGuiKey_RightArrow:
				return GLFW_KEY_RIGHT;
			case ImGuiKey_UpArrow:
				return GLFW_KEY_UP;
			case ImGuiKey_DownArrow:
				return GLFW_KEY_DOWN;
			case ImGuiKey_PageDown:
				return GLFW_KEY_PAGE_DOWN;
			case ImGuiKey_PageUp:
				return GLFW_KEY_PAGE_UP;
			case ImGuiKey_Home:
				return GLFW_KEY_HOME;
			case ImGuiKey_End:
				return GLFW_KEY_END;
			case ImGuiKey_Insert:
				return GLFW_KEY_INSERT;
			case ImGuiKey_Delete:
				return GLFW_KEY_DELETE;
			case ImGuiKey_Backspace:
				return GLFW_KEY_BACKSPACE;
			case ImGuiKey_Space:
				return GLFW_KEY_SPACE;
			case ImGuiKey_Enter:
				return GLFW_KEY_ENTER;
			case ImGuiKey_Escape:
				return GLFW_KEY_ESCAPE;
			case ImGuiKey_A:
				return GLFW_KEY_A;
			case ImGuiKey_C:
				return GLFW_KEY_C;
			case ImGuiKey_V:
				return GLFW_KEY_V;
			case ImGuiKey_X:
				return GLFW_KEY_X;
			case ImGuiKey_Y:
				return GLFW_KEY_Y;
			case ImGuiKey_Z:
				return GLFW_KEY_Z;
			default:
				return GLFW_KEY_UNKNOWN;
			}
		},
		"getText", [](ImGuiTextEditCallbackData* data) -> std::string { return std::string(data->Buf, data->BufTextLen); },
		"cursorPos", &ImGuiTextEditCallbackData::CursorPos
	);

	// debugger
	lua["debugger"] = lua.create_table_with(
		// Originally this was called "getLog", so in lua you'd do `debugger.getLog()`, but it was using it as a property getter instead of a function getter
		"getLogs", []() -> sol::as_table_t<std::vector<Log>> { return Debugger::getLog(); },
		"addLog", [](DebugLevel level, std::string message) { Debugger::addLog(level, message); },
		"clearLog", []() { Debugger::clearLog(); }
	);
	lua.new_usertype<Log>("log",
		"new", sol::no_constructor,
		"level", &Log::level,
		"message", &Log::message
	);
}
