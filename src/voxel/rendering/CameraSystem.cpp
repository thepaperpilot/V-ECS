#include "CameraSystem.h"
#include "VoxelRenderer.h"
#include "../../engine/GLFWEvents.h"
#include "../../events/EventManager.h"
#include "../../rendering/CameraComponent.h"

#include <glm\ext\matrix_clip_space.hpp>
#include <glm/gtx/string_cast.hpp>

using namespace vecs;

void CameraSystem::init() {
    // Set window resize event listener so we know to re-calculate our projection matrices
    EventManager::addListener(this, &CameraSystem::windowResize);

    // Create entity query so we can get our cameras
    cameras.filter.with(typeid(CameraComponent));
    world->addQuery(&cameras);
}

void CameraSystem::update() {
    for (auto archetypes : cameras.matchingArchetypes) {
        for (auto component : *archetypes->getComponentList(typeid(CameraComponent))) {
            CameraComponent* camera = static_cast<CameraComponent*>(component);

            // Re-calculate projection matrices if necessary
            if (camera->projDirty) {
                // TODO not all cameras may be attached to voxel renderer
                float aspectRatio = voxelRenderer->renderer->swapChainExtent.width /
                    (float)voxelRenderer->renderer->swapChainExtent.height;
                camera->projection = glm::perspective(glm::radians(45.0f), aspectRatio, camera->near, camera->far);
                // Flip projection's yy component because opengl has is flipped compared to vulkan and glm assumes opengl
                camera->projection[1][1] *= -1;
                camera->projDirty = false;
            }

            if (camera->isDirty) {
                // TODO not all cameras may be attached to voxel renderer
                voxelRenderer->markAllBuffersDirty();
                camera->isDirty = false;
            }
        }
    }
}

void CameraSystem::windowResize(WindowResizeEvent* event) {
    for (auto archetypes : cameras.matchingArchetypes) {
        for (auto component : *archetypes->getComponentList(typeid(CameraComponent))) {
            static_cast<CameraComponent*>(component)->projDirty = true;
        }
    }
}
