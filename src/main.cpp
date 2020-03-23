#ifdef NDEBUG
#pragma comment(linker, "/SUBSYSTEM:windows /ENTRY:mainCRTStartup")
#endif

#include "engine/Engine.h"
#include "ecs/World.h"
#include "voxel/VoxelWorld.h"
#include "voxel/data/ChunkComponent.h"
#include "voxel/data/BlockComponent.h"
#include "voxel/data/ChunkBuilder.h"
#include "voxel/rendering/MeshComponent.h"
#include "gui/GUIRenderer.h"

#include <thread>

using namespace vecs;

Engine app;

uint16_t chunksPerAxis = 8;
uint16_t chunkSize = 16;

VoxelWorld* game;
World* loading;

void preCleanup() {
    // Cleanup our worlds
    if (loading->activeWorld) loading->cleanup();
    else {
        game->cleanup();
    }
}

void loadGame() {
    // Set up temporary chunks
    int totalChunks = chunksPerAxis * chunksPerAxis * chunksPerAxis;
    int totalBlocks = totalChunks * chunkSize * chunkSize * chunkSize;
    ChunkBuilder::init();
    ChunkBuilder chunkBuilder(game, &game->voxelRenderer.blockLoader, 1227, chunkSize);
    for (int32_t x = -chunksPerAxis / 2; x < chunksPerAxis / 2; x++) {
        for (int32_t y = -chunksPerAxis / 2; y < chunksPerAxis / 2; y++) {
            for (int32_t z = -chunksPerAxis / 2; z < chunksPerAxis / 2; z++) {
                chunkBuilder.fillChunk(x, y, z);
            }
        }
    }

    // Change world and cleanup loading world
    app.setWorld(game, false, true);
}

int main() {
    app.preCleanup = preCleanup;
    app.init();

    // Create thread to load our game screen
    // TODO move this to the other thread once you can give the thread its own VkQeueue
    game = new VoxelWorld(&app.renderer, chunkSize);
    game->init(app.device, app.window);
    std::thread loadingThread(loadGame);

    // Create loading screen world
    GUIRenderer loadingScreenRenderer;
    loading = new World(&app.renderer);
    loading->subrenderers.insert(&loadingScreenRenderer);
    app.setWorld(loading);

    try {
        app.run();
    } catch (const std::exception & e) {
        std::cerr << e.what() << std::endl;
        return EXIT_FAILURE;
    }

    loadingThread.join();

    return EXIT_SUCCESS;
}
