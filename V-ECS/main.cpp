#ifdef NDEBUG
#pragma comment(linker, "/SUBSYSTEM:windows /ENTRY:mainCRTStartup")
#endif

#include "engine/Engine.h"
#include "ecs/World.h"
#include "voxel/VoxelWorld.h"
#include "voxel/components/ChunkComponent.h"
#include "voxel/components/BlockComponent.h"
#include "voxel/rendering/MeshComponent.h"
#include "voxel/components/ChunkBuilder.h"
#include "gui/GUIRenderer.h"

#include <thread>

using namespace vecs;

Engine app;

uint16_t chunksPerAxis = 8;
uint16_t chunkSize = 16;

VoxelWorld game(chunkSize);
World loading;

void preCleanup() {
    // Cleanup our worlds
    if (loading.activeWorld) loading.cleanup();
    else {
        game.cleanup();
    }
}

void loadGame() {
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

    // Setup world and run first update so the meshes are generated,
    // and only then do we switch the world to the new one
    app.setupWorld(&game);

    // Change world and cleanup loading world
    app.setWorld(&game, false, true);
}

int main() {
    app.preCleanup = preCleanup;

    // Create thread to load our chunks
    std::thread loadingThread(loadGame);

    // Create loading screen world
    GUIRenderer loadingScreenRenderer;
    loading.renderer.registerSubRenderer(&loadingScreenRenderer);
    app.setWorld(&loading);

    try {
        app.run();
    } catch (const std::exception & e) {
        std::cerr << e.what() << std::endl;
        return EXIT_FAILURE;
    }

    if (loadingThread.joinable())
        loadingThread.join();

    return EXIT_SUCCESS;
}
