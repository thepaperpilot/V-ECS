#include "CameraSystem.h"
#include "CameraComponent.h"
#include "VoxelRenderer.h"
#include "../../engine/GLFWEvents.h"
#include "../../events/EventManager.h"

#include <glm\ext\matrix_clip_space.hpp>
#include <glm/gtx/string_cast.hpp>

using namespace vecs;

void CameraSystem::init() {
    // Set window resize event listener so we know to re-calculate our projection matrices
    EventManager::addListener(this, &CameraSystem::windowResize);

    // Create entity query so we can get our cameras
    cameras.filter.with(typeid(CameraComponent));
    cameras.onEntityAdded = EntityQuery::bind(this, &CameraSystem::onCameraAdded);
    world->addQuery(&cameras);
}

void CameraSystem::update() {
    for (uint32_t entity : cameras.entities) {
        CameraComponent* camera = world->getComponent<CameraComponent>(entity);
        
        // Re-calculate projection matrices if necessary
        if (camera->projDirty) {
            float aspectRatio = voxelRenderer->renderer->swapChainExtent.width /
                         (float)voxelRenderer->renderer->swapChainExtent.height;
            camera->projection = glm::perspective(glm::radians(45.0f), aspectRatio, camera->near, camera->far);
            // Flip projection's yy component because opengl has is flipped compared to vulkan and glm assumes opengl
            camera->projection[1][1] *= -1;
            camera->projDirty = false;
        }

        if (camera->isDirty) {
            voxelRenderer->markAllBuffersDirty();
            camera->isDirty = false;
        }
    }
}

void CameraSystem::windowResize(WindowResizeEvent* event) {
    for (uint32_t entity : cameras.entities) {
        world->getComponent<CameraComponent>(entity)->projDirty = true;
    }
}

void CameraSystem::onCameraAdded(uint32_t entity) {
    // Point our push constants to the camera's data points
    CameraComponent* camera = world->getComponent<CameraComponent>(entity);
    voxelRenderer->model = &camera->model;
    voxelRenderer->view = &camera->view;
    voxelRenderer->projection = &camera->projection;
}
