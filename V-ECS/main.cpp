#include "engine/Engine.h"
#include "ecs/World.h"
#include "voxel/VoxelWorld.h"
#include "voxel/components/ChunkComponent.h"
#include "voxel/components/BlockComponent.h"
#include "voxel/rendering/MeshComponent.h"
#include "voxel/components/ChunkBuilder.h"

using namespace vecs;

Engine app;

uint16_t chunksPerAxis = 16;
uint16_t chunkSize = 16;

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
    ChunkBuilder chunkBuilder(&game, chunkSize);
    for (int32_t x = -chunksPerAxis / 2; x < chunksPerAxis / 2; x++) {
        for (int32_t y = -chunksPerAxis / 2; y < chunksPerAxis / 2; y++) {
            for (int32_t z = -chunksPerAxis / 2; z < chunksPerAxis / 2; z++) {
                chunkBuilder.fillChunk(x, y, z);
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
