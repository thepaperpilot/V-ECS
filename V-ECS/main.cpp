#include "engine/Engine.h"
#include "ecs/World.h"
#include "voxel/VoxelWorld.h"
#include "voxel/components/ChunkComponent.h"
#include "voxel/components/BlockComponent.h"
#include "voxel/rendering/MeshComponent.h"
#include "voxel/components/ChunkBuilder.h"

using namespace vecs;

Engine app;

uint16_t chunksPerAxis = 6;
uint16_t chunkSize = 32;

VoxelWorld game(chunkSize);

void preCleanup() {
    // Cleanup our worlds
    game.cleanup();
}

int main() {
    app.preCleanup = preCleanup;

    // Set up temporary chunks
    int totalChunks = chunksPerAxis * chunksPerAxis * chunksPerAxis;
    int totalBlocks = totalChunks * chunkSize * chunkSize * chunkSize;
    game.reserveEntities(totalChunks);
    ChunkBuilder chunkBuilder(&game, chunkSize);
    for (int32_t x = -chunksPerAxis / 2; x < chunksPerAxis / 2; x++) {
        for (int32_t y = -chunksPerAxis / 2; y < chunksPerAxis / 2; y++) {
            for (int32_t z = -chunksPerAxis / 2; z < chunksPerAxis / 2; z++) {
                uint32_t chunk = game.createEntity();
                ChunkComponent* chunkComponent;
                game.addComponent(chunk, chunkComponent = new ChunkComponent);
                chunkComponent->x = x;
                chunkComponent->y = y;
                chunkComponent->z = z;
                game.addComponent(chunk, new MeshComponent);
                chunkBuilder.fillChunk(chunk, chunkComponent);
            }
        }
    }

    // Set initial world
    app.setupWorld(&game);

    try {
        app.run();
    } catch (const std::exception & e) {
        std::cerr << e.what() << std::endl;
        return EXIT_FAILURE;
    }

    return EXIT_SUCCESS;
}
