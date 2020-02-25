#include "ChunkBuilder.h"

#include "ChunkComponent.h"
#include "BlockComponent.h"
#include "BlockLoader.h"
#include "ChunkSystem.h"
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

struct ArchetypeHandle {
public:
	ArchetypeHandle(std::string id) {
		this->id = id;
	};

	void addEntity(uint32_t entity, glm::u16vec3 internalPos) {
		entities.emplace_back(entity);
		entityPositions.emplace_back(internalPos);
	}

	// TODO to support multithreading, lock the entities resource during this function
	void commit(BlockLoader* blockLoader, uint16_t chunkSize, glm::i32vec3 chunkPos, Octree<uint32_t>* blocks, MeshComponent* mesh) {
		if (archetype == nullptr)
			archetype = blockLoader->getArchetype(id);
		archetype->addEntities(entities);
		BlockComponent* blockComponent = archetype->getSharedComponent<BlockComponent>();
		for (auto internalPos : entityPositions)
			ChunkSystem::addBlockFaces(blockComponent, internalPos, chunkSize, chunkPos, blocks, mesh);
		entities.clear();
		entityPositions.clear();
	}

private:
	std::string id;
	Archetype* archetype = nullptr;
	std::vector<uint32_t> entities;
	std::vector<glm::u16vec3> entityPositions;
};

ArchetypeHandle* getArchetype(std::string id) {
	return new ArchetypeHandle(id);
}

struct LuaChunkHandle {
public:
	LuaChunkHandle(uint16_t chunkSize, BlockLoader* blockLoader) : blocks(chunkSize) {
		this->chunkSize = chunkSize;
		this->blockLoader = blockLoader;
	}

	void setBlock(uint32_t blockPos, ArchetypeHandle* archetype) {
		// Lua starts at 1, so we need to deal with 1 less than blockPos
		blockPos--;
		uint16_t x = (blockPos / (chunkSize * chunkSize));
		uint16_t y = blockPos % (chunkSize * chunkSize) / chunkSize;
		uint16_t z = blockPos % chunkSize;
		
		blocks.set(x, y, z, archetype);
	}

	void commit(World* world, ChunkComponent* chunk, MeshComponent* mesh) {
		std::unordered_set<ArchetypeHandle*> archetypes;
		for (uint16_t z = 0; z < chunkSize; z++) {
			for (uint16_t y = 0; y < chunkSize; y++) {
				for (uint16_t x = 0; x < chunkSize; x++) {
					ArchetypeHandle* archetype = blocks.at(x, y, z);
					if (archetype != nullptr) {
						uint32_t entity = world->createEntities(1);
						chunk->blocks.set(x, y, z, entity);
						archetype->addEntity(entity, { x, y, z });
						archetypes.insert(archetype);
					}
				}
			}
		}

		for (auto archetype : archetypes) {
			archetype->commit(blockLoader, chunkSize, { chunk->x, chunk->y, chunk->z }, &chunk->blocks, mesh);
		}
	}

private:
	uint16_t chunkSize;
	BlockLoader* blockLoader;
	Octree<ArchetypeHandle*> blocks;
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
			.addFunction("getArchetype", &getArchetype)
		.endNamespace()
		.beginClass<LuaChunkHandle>("chunkHandle")
			.addFunction("setBlock", &LuaChunkHandle::setBlock)
		.endClass()
		.beginClass<LuaNoiseHandle>("noiseHandle")
			.addFunction("getNoiseSet", &LuaNoiseHandle::getNoiseSet)
			.addProperty("seed", &LuaNoiseHandle::seed, false)
		.endClass()
		.beginClass<ArchetypeHandle>("archetypeHandle")
		.endClass();

	for (auto resource : getResources("terrain", ".lua")) {
		if (luaL_loadfile(L, resource.c_str()) || lua_pcall(L, 0, 0, 0)) {
			std::cout << "Failed to load terrain generator " << resource << ":" << std::endl;
			std::cout << lua_tostring(L, -1) << std::endl;
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

	chunkHandle.commit(world, chunkComponent, meshComponent);
}
