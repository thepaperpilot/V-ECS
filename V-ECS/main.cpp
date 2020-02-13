#include "engine/Engine.h"
#include "ecs/World.h"
#include "voxel/VoxelWorld.h"
#include "voxel/components/ChunkComponent.h"
#include "voxel/components/BlockComponent.h"
#include "voxel/rendering/MeshComponent.h"

using namespace vecs;

Engine app;

VoxelWorld game;

void preCleanup() {
    // Cleanup our worlds
    game.cleanup();
}

int main() {
    app.preCleanup = preCleanup;

    // Set up temporary chunk
    uint32_t chunk = game.createEntity();
    game.addComponent(chunk, new ChunkComponent);
    game.addComponent(chunk, new MeshComponent);

    uint32_t block = game.createEntity();
    game.addComponent(block, new BlockComponent);

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
