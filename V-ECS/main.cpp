#include "engine/Engine.h"
#include "ecs/World.h"
#include "voxel/VoxelWorld.h"
#include "voxel/ChunkComponent.h"
#include "voxel/BlockComponent.h"
#include "rendering/MeshComponent.h"

using namespace vecs;

Engine app;

World title(&app);
VoxelWorld game(&app);

void preCleanup() {
    // Cleanup our worlds
    title.cleanup();
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

    try {
        app.run(&game);
    } catch (const std::exception & e) {
        std::cerr << e.what() << std::endl;
        return EXIT_FAILURE;
    }

    return EXIT_SUCCESS;
}
