#pragma once

#include "../../ecs/World.h"

#include "lua.hpp"
#include "lauxlib.h"
#include "lualib.h"
#include <LuaBridge/LuaBridge.h>

#include "hastyNoise.h"

#include <queue>

namespace vecs {

	// Forward Declarations
	class Archetype;
	class BlockLoader;

	class ChunkBuilder {
	public:
		inline static size_t fastestSimd = 0;

		static void init() {
			// Setup our noise generator
			HastyNoise::loadSimd("./simd");
			fastestSimd = HastyNoise::GetFastestSIMD();
		}

		ChunkBuilder(World* world, BlockLoader* blockLoader, int seed, uint16_t chunkSize);

		void fillChunk(int32_t x, int32_t y, int32_t z);
		
	private:
		World* world;
		BlockLoader* blockLoader;

		int seed;
		uint16_t chunkSize;

		Archetype* chunkArchetype;
		std::vector<Component*>* chunkComponents;
		std::vector<Component*>* meshComponents;

		std::multimap<int, luabridge::LuaRef> generators;
	};
}
