#include "World.h"

#include <hastyNoise/hastyNoise.h>

#include <glm\ext\matrix_clip_space.hpp>
#include <glm/gtx/string_cast.hpp>

#include <stb/stb_image.h>
// rectpack2d:
#include "finders_interface.h"

#include <imgui.h>
#include <examples\imgui_impl_glfw.h>

#include "../events/LuaEvents.h"
#include "../lib/FrustumCull.h"

using namespace vecs;
using namespace rectpack2D;
using namespace ImGui;

// Simple container for storing pixel data and the name of the file it came from
struct SubTexture {
	stbi_uc* pixels;
	std::string filename;
};

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

Archetype* World::getArchetype(std::unordered_set<std::string> componentTypes, std::unordered_map<std::string, sol::table>* sharedComponents) {
	auto itr = std::find_if(archetypes.begin(), archetypes.end(), [&componentTypes, &sharedComponents](Archetype* archetype) {
		return archetype->componentTypes == componentTypes && (sharedComponents == nullptr || archetype->sharedComponents == sharedComponents);
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
}

void World::setupState(Engine* engine) {
	lua.open_libraries(sol::lib::base, sol::lib::package, sol::lib::math, sol::lib::string);

	// Utility functions
	lua["print"] = [](std::string s) { std::cout << "[LUA] " << s << std::endl; };
	lua["getResources"] = [](std::string path, std::string extension) -> std::vector<std::string> {
		std::vector<std::string> resources;
		auto filepath = std::filesystem::path("./resources/" + path).make_preferred();
		for (const auto& entry : std::filesystem::directory_iterator(filepath))
			if (!entry.is_directory() && entry.path().extension() == extension)
				resources.push_back(entry.path().string());
		return resources;
	};
	lua["loadWorld"] = [engine](sol::table worldConfig) { engine->setWorld(new World(engine, worldConfig)); };

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
	lua["sizes"] = lua.create_table_with(
		"float", sizeof(float),
		"vec3", sizeof(glm::vec3),
		"mat4", sizeof(glm::mat4)
	);

	// time namespace
	lua["time"] = lua.create_table_with(
		"getDeltaTime", [this]() -> float { return deltaTime; }
	);

	// glfw namespace
	lua["glfw"] = lua.create_table_with(
		"hideCursor", [window = engine->window]() { glfwSetInputMode(window, GLFW_CURSOR, GLFW_CURSOR_DISABLED); },
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

	// keys namespace
	lua["keys"] = lua.create_table_with(
		"SPACE", GLFW_KEY_SPACE,
		"APOSTROPHE", GLFW_KEY_APOSTROPHE,
		"COMMA", GLFW_KEY_COMMA,
		"MINUS", GLFW_KEY_MINUS,
		"PERIOD", GLFW_KEY_PERIOD,
		"SLASH", GLFW_KEY_SLASH,
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
		"SEMICOLON", GLFW_KEY_SEMICOLON,
		"EQUAL", GLFW_KEY_EQUAL,
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
		"LEFT_BRACKET", GLFW_KEY_LEFT_BRACKET,
		"BACKSLASH", GLFW_KEY_BACKSLASH,
		"RIGHT_BRACKET", GLFW_KEY_RIGHT_BRACKET,
		"GRAVE", GLFW_KEY_GRAVE_ACCENT,
		"ESCAPE", GLFW_KEY_ESCAPE,
		"ENTER", GLFW_KEY_ENTER,
		"TAB", GLFW_KEY_TAB,
		"BACKSPACE", GLFW_KEY_BACKSPACE,
		"INSERT", GLFW_KEY_INSERT,
		"DELETE", GLFW_KEY_DELETE,
		"RIGHT", GLFW_KEY_RIGHT,
		"LEFT", GLFW_KEY_LEFT,
		"DOWN", GLFW_KEY_DOWN,
		"UP", GLFW_KEY_UP,
		"LEFT_SHIFT", GLFW_KEY_LEFT_SHIFT,
		"LEFT_CONTROL", GLFW_KEY_LEFT_CONTROL,
		"LEFT_ALT", GLFW_KEY_LEFT_ALT,
		"LEFT_SUPER", GLFW_KEY_LEFT_SUPER,
		"RIGHT_SHIFT", GLFW_KEY_RIGHT_SHIFT,
		"RIGHT_CONTROL", GLFW_KEY_RIGHT_CONTROL,
		"RIGHT_ALT", GLFW_KEY_RIGHT_ALT,
		"RIGHT_SUPER", GLFW_KEY_RIGHT_SUPER
	);

	// glm functions, w/o namespace to make it more glsl-like
	lua["normalize"] = [](glm::vec3 v) -> glm::vec3 { return glm::normalize(v); };
	lua["radians"] = [](float v) -> float { return glm::radians(v); };
	lua["sin"] = [](float v) -> float { return glm::sin(v); };
	lua["cos"] = [](float v) -> float { return glm::cos(v); };
	lua["cross"] = [](glm::vec3 first, glm::vec3 second) -> glm::vec3 { return glm::cross(first, second); };
	lua["perspective"] = [](float fov, float aspectRatio, float nearPlane, float farPlane) -> glm::mat4 { return glm::perspective(fov, aspectRatio, nearPlane, farPlane); };
	lua.new_usertype<glm::vec3>("vec3",
		sol::constructors<glm::vec3(), glm::vec3(float), glm::vec3(float, float, float)>(),
		"x", &glm::vec3::x,
		"y", &glm::vec3::y,
		"z", &glm::vec3::z,
		sol::meta_function::multiplication, sol::overload(
			[](const glm::vec3& v1, const glm::vec3& v2) -> glm::vec3 { return v1 * v2; },
			[](const glm::vec3& v1, float f) -> glm::vec3 { return v1 * f; },
			[](float f, const glm::vec3& v1) -> glm::vec3 { return f * v1; }
		),
		sol::meta_function::to_string, [](const glm::vec3& v1) -> std::string { return glm::to_string(v1); }
	);
	lua.new_usertype<glm::mat4>("mat4",
		sol::constructors<glm::mat4()>(),
		"translate", sol::factories([](glm::vec3 translation) -> glm::mat4 { return glm::translate(glm::mat4(1.0), translation); }),
		// TODO find better way to make setters/getters
		"get", [](const glm::mat4& m1, int idx1, int idx2) -> float { return m1[idx1][idx2]; },
		"set", [](glm::mat4& m1, int idx1, int idx2, float value) { m1[idx1][idx2] = value; },
		sol::meta_function::to_string, [](const glm::mat4& m1) -> std::string { return glm::to_string(m1); }
	);

	lua.new_usertype<Frustum>("frustum",
		sol::constructors<Frustum(glm::mat4)>(),
		"isBoxVisible", &Frustum::IsBoxVisible
	);

	// ecs
	lua.new_usertype<Archetype>("archetype",
		"new", sol::factories(
			[this](std::unordered_set<std::string> required) -> Archetype* { return this->getArchetype(required); },
			[this](std::unordered_set<std::string> required, std::unordered_map<std::string, sol::table>* sharedComponents) -> Archetype* { return this->getArchetype(required, sharedComponents); }
		),
		"isEmpty", [](const Archetype& archetype) -> bool { return archetype.componentTypes.size(); },
		"getComponents", [](Archetype& archetype, std::string componentType) -> std::vector<sol::table> { return *archetype.getComponentList(componentType); },
		"createEntity", [](Archetype& archetype) -> std::pair<uint32_t, size_t> { return archetype.createEntities(1); },
		"createEntities", &Archetype::createEntities
	);
	lua.new_usertype<EntityQuery>("query"
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
		"archetypes", &EntityQuery::matchingArchetypes
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
		"WindowRefresh", &(new WindowRefreshLuaEvent(&lua))->event,
		"WindowResize", &(new WindowResizeLuaEvent(&lua))->event,
		"MouseMove", &(new MouseMoveLuaEvent(&lua))->event,
		"LeftMousePress", &(new LeftMousePressLuaEvent(&lua))->event,
		"LeftMouseRelease", &(new LeftMouseReleaseLuaEvent(&lua))->event,
		"RightMousePress", &(new RightMousePressLuaEvent(&lua))->event,
		"RightMouseRelease", &(new RightMouseReleaseLuaEvent(&lua))->event,
		"VerticalScroll", &(new VerticalScrollLuaEvent(&lua))->event,
		"HorizontalScroll", &(new HorizontalScrollLuaEvent(&lua))->event,
		"KeyPress", &(new KeyPressLuaEvent(&lua))->event,
		"KeyRelease", &(new KeyReleaseLuaEvent(&lua))->event
	);

	// noise
	lua.new_usertype<HastyNoise::NoiseSIMD>("noise",
		"new", sol::factories(
			[](HastyNoise::NoiseType noiseType, int seed, float frequency) -> HastyNoise::NoiseSIMD* {
				HastyNoise::NoiseSIMD* noise = HastyNoise::details::CreateNoise(seed, Engine::fastestSimd);
				noise->SetNoiseType(noiseType);
				noise->SetFrequency(frequency);
				return noise;
			}
		),
		"getNoiseSet", [](HastyNoise::NoiseSIMD noise, int chunkX, int chunkY, int chunkZ, int chunkSize) -> std::vector<float> {
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
		"pushConstant", [](SubRenderer renderer, VkShaderStageFlags shaderStage, int offset, size_t size, void* constant) {
			vkCmdPushConstants(renderer.activeCommandBuffer, renderer.pipelineLayout, shaderStage, offset, size, constant);
		},
		"draw", [](SubRenderer renderer, Model* model) { model->draw(renderer.activeCommandBuffer, renderer.pipelineLayout); },
		"drawVertices", [](SubRenderer renderer, Buffer* vertexBuffer, Buffer* indexBuffer, int indexCount) {
			VkBuffer vertexBuffers[] = { vertexBuffer->buffer };
			VkDeviceSize offsets[] = { 0 };
			vkCmdBindVertexBuffers(renderer.activeCommandBuffer, 0, 1, vertexBuffers, offsets);
			vkCmdBindIndexBuffer(renderer.activeCommandBuffer, indexBuffer->buffer, 0, VK_INDEX_TYPE_UINT32);
			vkCmdDrawIndexed(renderer.activeCommandBuffer, indexCount, 1, 0, 0, 0);
		}
	);
	lua.new_usertype<Model>("model",
		"new", sol::constructors<Model(SubRenderer*, const char*), Model(SubRenderer*, const char*, VkShaderStageFlagBits, std::vector<MaterialComponent>)>()
	);
	lua.new_usertype<Texture>("texture",
		sol::constructors<Texture(SubRenderer*, const char*)>(),
		"createStitched", [this](SubRenderer* subrenderer, std::vector<std::string> images) -> sol::table {
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
				subtextures[texIdx].filename = image;
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
					"t", (rect.y + rect.h) / (float)texSize.h
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
			new Texture(subrenderer, pixels, texSize.w, texSize.h);
			stbi_image_free(pixels);

			return map;
		}
	);
	lua.new_usertype<Buffer>("buffer",
		"new", sol::factories(
			[device = this->device](VkBufferUsageFlags usageFlags, VkDeviceSize size) -> Buffer* {
				return &device->createBuffer(size, usageFlags, VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT);
			}
		),
		"setData", [](Buffer buffer, void* data) { buffer.copyTo(0, data); }
	);

	// imgui
	lua["ig"] = lua.create_table_with(
		"createFontTexture", [](SubRenderer* subrenderer) {
			// Create our font texture from ImGUI's pixel data
			ImGuiIO& io = ImGui::GetIO();
			unsigned char* pixels;
			int width, height;
			io.Fonts->GetTexDataAsRGBA32(&pixels, &width, &height);
			Texture* fontTexture = new Texture(subrenderer, pixels, width, height);
			io.Fonts->TexID = (ImTextureID)(intptr_t)fontTexture->image;
			subrenderer->immutableSamplers.emplace_back(fontTexture->sampler);
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
			if (vkFlushMappedMemoryRanges(device->logical, 2, range) != VK_SUCCESS) {
				throw std::runtime_error("failed to flush memory!");
			}
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
		"setWindowFontScale", [](float scale) { SetWindowFontScale(scale); },
		"beginWindow", [](std::string title, bool* p_open, std::vector<ImGuiWindowFlags> flags) {
			ImGuiWindowFlags windowFlags = 0;
			for (auto flag : flags)
				windowFlags = windowFlags | flag;
			Begin(title.c_str(), p_open, windowFlags);
		},
		"endWindow", []() { End(); },
		"text", [](std::string text) { Text(text.c_str()); },
		"button", [](std::string text) -> bool { return Button(text.c_str()); },
		"separator", []() { Separator(); },
		"showDemoWindow", []() { ShowDemoWindow(); }
	);
}
