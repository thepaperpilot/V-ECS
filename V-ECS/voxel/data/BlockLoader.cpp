#include "BlockLoader.h"

#include "../../ecs/World.h"
#include "../../ecs/Archetype.h"
#include "../../engine/Device.h"
#include "BlockComponent.h"

#include "lua.hpp"
#include "lauxlib.h"
#include "lualib.h"
#include <LuaBridge/LuaBridge.h>
#include "../../util/LuaUtils.h"

#include <stb_image.h>
// rectpack2d:
#include "finders_interface.h"

#include <glm/glm.hpp>

using namespace vecs;
using namespace luabridge;
using namespace rectpack2D;

struct SubTexture {
	stbi_uc* pixels;
	int texWidth;
	int texHeight;
	
	// Used to look up which UV rectangle this texture is associated with
	uint16_t texIdx;

	// These'll be set after calculating how best to fit every texture into one
	glm::vec4 UVs;
};

// We create one large texture for all block textures so that our single renderer
// only needs the one image. Since the number of blocks is dynamic
// (and not necessarily the same sized textures), there's no way
// to create an array of textures or a textureArray
// In theory at some point I might make VoxelRenderer a LuaRenderer and make it so
// you could add things like normal maps and stuff completely through lua
VkDescriptorImageInfo BlockLoader::loadBlocks(Device* device, VkQueue copyQueue) {
	auto L = getState();

	using spaces_type = empty_spaces<false, default_empty_spaces>;
	using rect_type = output_rect_t<spaces_type>;

	uint16_t texIdx = 0;
	std::map<std::string, SubTexture*> textures;
	std::vector<rect_type> rects;

	// Load all sub-textures
	auto resources = getResources("blocks", ".lua");
	std::vector<LuaRef> blocks;
	blocks.reserve(resources.size());
	for (auto resource : resources) {
		if (luaL_loadfile(L, resource.c_str()) || lua_pcall(L, 0, 0, 0)) {
			std::cout << "Failed to load block " << resource << ":" << std::endl;
			std::cout << lua_tostring(L, -1) << std::endl;
			continue;
		}

		LuaRef block = getGlobal(L, "block");
		blocks.emplace_back(block);

		LuaRef tex = block["texture"];
		if (tex.isString()) {
			std::string texture = tex.cast<std::string>();
			if (!textures.count(texture)) {
				SubTexture* subtexture = new SubTexture;
				int texChannels;
				subtexture->pixels = stbi_load(("resources/textures/" + texture).c_str(), &subtexture->texWidth, &subtexture->texHeight, &texChannels, STBI_rgb_alpha);
				subtexture->texIdx = texIdx++;
				textures.emplace(texture, subtexture);
				rects.emplace_back(0, 0, subtexture->texWidth, subtexture->texHeight);
			}
		}
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

	// Calculate UVs for each sub-texture and create the image data for our main texture
	unsigned char* pixels = new unsigned char[texSize.w * texSize.h * 4];
	for (auto tex : textures) {
		auto rect = rects[tex.second->texIdx];
		tex.second->UVs = {
			rect.x / (float)texSize.w, rect.y / (float)texSize.h,
			(rect.x + rect.w) / (float)texSize.w, (rect.y + rect.h) / (float)texSize.h
		};
		for (int y = 0; y < rect.h; y++) {
			int mainOffset = (rect.y + y) * (texSize.w * 4) + rect.x * 4;
			int subOffset = y * (rect.w * 4);
			for (int x = 0; x < rect.w * 4; x++) {
				pixels[mainOffset + x] = tex.second->pixels[subOffset + x];
			}
		}
	}

	// Create our texture and release stbi pixel data
	stitchedTexture.init(device, copyQueue, pixels, texSize.w, texSize.h);
	stbi_image_free(pixels);
	/*
	for (auto tex : textures) {
		stbi_image_free(tex.second->pixels);
	}
	*/

	// Add UVs to each block archetype
	for (auto block : blocks) {
		std::string id = getString(block["id"]);
		std::unordered_map<std::type_index, Component*> sharedComponents;

		LuaRef tex = block["texture"];
		if (tex.isString()) {
			auto UVs = textures[tex.cast<std::string>()]->UVs;
			BlockComponent* blockComponent = new BlockComponent;
			blockComponent->top = { UVs.p, UVs.q, UVs.s, UVs.t };
			blockComponent->bottom = { UVs.p, UVs.t, UVs.s, UVs.q };
			blockComponent->front = blockComponent->top;
			blockComponent->back = blockComponent->bottom;
			blockComponent->left = blockComponent->top;
			blockComponent->right = blockComponent->top;

			sharedComponents.insert({ typeid(BlockComponent), blockComponent });
		}

		this->sharedComponents.emplace(id, sharedComponents);
	}

	return stitchedTexture.descriptor;
}

Archetype* BlockLoader::getArchetype(std::string id) {
	return world->getArchetype({}, &sharedComponents[id]);
}

void BlockLoader::cleanup() {
	stitchedTexture.cleanup();
}
