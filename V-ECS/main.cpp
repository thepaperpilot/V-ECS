#include "engine/Engine.h"
#include "ecs/World.h"
#include "voxel/VoxelWorld.h"

using namespace vecs;

Engine app;

World title;
VoxelWorld game(&app);

void preCleanup() {
    // Cleanup our worlds
    title.cleanup();
    game.cleanup();
}

int main() {
    app.preCleanup = preCleanup;

    try {
        app.run(&game);
    } catch (const std::exception & e) {
        std::cerr << e.what() << std::endl;
        return EXIT_FAILURE;
    }

    return EXIT_SUCCESS;
}
