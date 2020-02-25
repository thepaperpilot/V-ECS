#include "ChunkBuilder.h"

#include "ChunkComponent.h"
#include "BlockComponent.h"
#include "BlockLoader.h"
#include "../rendering/MeshComponent.h"
#include "../../ecs/Archetype.h"

#include "../../util/LuaUtils.h"

using namespace vecs;
using namespace luabridge;

struct LuaNoiseHandle {
public:
	HastyNoise::NoiseSIMD* noise;
	int seed;

	void init(int seed, size_t fastestSimd, HastyNoise::NoiseType noiseType) {
		this->seed = seed;
		noise = HastyNoise::details::CreateNoise(seed, fastestSimd);
		noise->SetNoiseType(noiseType);
	};

	std::vector<float> getNoiseSet(int32_t chunkX, int32_t chunkY, int32_t chunkZ, uint16_t chunkSize) {
		HastyNoise::FloatBuffer floatBuffer = noise->GetNoiseSet(chunkX * chunkSize, chunkY * chunkSize, chunkZ * chunkSize, chunkSize, chunkSize, chunkSize);
		std::vector<float> points(floatBuffer.get(), floatBuffer.get() + chunkSize * chunkSize * chunkSize);
		return points;
	};
};

LuaNoiseHandle createSimplexNoise(int seed) {
	LuaNoiseHandle noise;
	noise.init(seed, ChunkBuilder::fastestSimd, HastyNoise::NoiseType::OpenSimplex2);
	return noise;
}

LuaNoiseHandle createCellularNoise(int seed) {
	LuaNoiseHandle noise;
	noise.init(seed, ChunkBuilder::fastestSimd, HastyNoise::NoiseType::Cellular);
	return noise;
}

struct LuaChunkHandle {
public:
	LuaChunkHandle(uint16_t chunkSize, BlockLoader* blockLoader) : blocks(chunkSize, "") {
		this->chunkSize = chunkSize;
		this->blockLoader = blockLoader;
	}

	void setBlock(uint32_t blockPos, std::string archetype) {
		// Lua starts at 1, so we need to deal with 1 less than blockPos
		blockPos--;
		uint16_t z = (blockPos / (chunkSize * chunkSize));
		uint16_t y = blockPos % (chunkSize * chunkSize) / chunkSize;
		uint16_t x = chunkSize - 1 - blockPos % chunkSize;
		
		blocks.set(x, y, z, archetype);
	}

	void commit(ChunkComponent* chunk) {
		for (uint16_t z = 0; z < chunkSize; z++) {
			for (uint16_t y = 0; y < chunkSize; y++) {
				for (uint16_t x = 0; x < chunkSize; x++) {
					std::string archetype = blocks.at(x, y, z);
					if (archetype != "") {
						chunk->blocks.set(x, y, z, blockLoader->getArchetype(archetype)->createEntities(1).first);
					}
				}
			}
		}
	}

private:
	uint16_t chunkSize;
	BlockLoader* blockLoader;
	Octree<std::string> blocks;
};

ChunkBuilder::ChunkBuilder(World* world, BlockLoader* blockLoader, int seed, uint16_t chunkSize) {
	this->world = world;
	this->blockLoader = blockLoader;
	this->seed = seed;
	this->chunkSize = chunkSize;

	chunkArchetype = world->getArchetype({ typeid(ChunkComponent), typeid(MeshComponent) });
	chunkComponents = chunkArchetype->getComponentList(typeid(ChunkComponent));
	meshComponents = chunkArchetype->getComponentList(typeid(MeshComponent));

	// Build our Lua bindings for world generation
	auto L = getState();
	getGlobalNamespace(L)
		.beginNamespace("noise")
			.addProperty("seed", &seed, false)
			.addFunction("createSimplex", &createSimplexNoise)
			.addFunction("createCellular", &createCellularNoise)
		.endNamespace()
		.beginNamespace("blocks")
			.addProperty("chunkSize", &this->chunkSize, false)
			//.addFunction("addArchetype", )
		.endNamespace()
		.beginClass<LuaChunkHandle>("chunkHandle")
			.addFunction("setBlock", &LuaChunkHandle::setBlock)
		.endClass()
		.beginClass<LuaNoiseHandle>("noiseHandle")
			.addFunction("getNoiseSet", &LuaNoiseHandle::getNoiseSet)
			.addProperty("seed", &LuaNoiseHandle::seed, false)
		.endClass();

	for (auto resource : getResources("terrain", ".lua")) {
		if (luaL_loadfile(L, resource.c_str()) || lua_pcall(L, 0, 0, 0)) {
			// TODO how to get the errors from luabridge?
			std::cout << "Failed to load terrain generator " << resource << std::endl;
			continue;
		}

		LuaRef terrain = getGlobal(L, "terrain");
		generators.emplace(getInt(terrain["priority"]), terrain);
	}
}

void ChunkBuilder::fillChunk(int32_t x, int32_t y, int32_t z) {
	size_t chunk = chunkArchetype->createEntities(1).second;

	ChunkComponent* chunkComponent = new ChunkComponent(chunkSize, x, y, z);
	chunkComponents->at(chunk) = chunkComponent;

	MeshComponent* meshComponent = new MeshComponent;
	meshComponent->minBounds = { x * chunkSize, y * chunkSize, z * chunkSize };
	meshComponent->maxBounds = { (x + 1) * chunkSize, (y + 1) * chunkSize, (z + 1) * chunkSize };
	meshComponents->at(chunk) = meshComponent;
	LuaChunkHandle chunkHandle(chunkSize, blockLoader);

	for (auto generator : generators) {
		try {
			generator.second["processChunk"](&chunkHandle, x, y, z);
		}
		catch (LuaException const& e) {
			std::cout << "[LUA] " << e.what() << std::endl;
		}
	}

	chunkHandle.commit(chunkComponent);
}
