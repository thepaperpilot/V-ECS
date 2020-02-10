#include "CameraSystem.h"
#include "CameraComponent.h"
#include "PushConstantComponent.h"
#include "../engine/GLFWEvents.h"
#include "../rendering/Renderer.h"
#include "../events/EventManager.h"

#include <glm\ext\matrix_clip_space.hpp>
#include <glm/gtx/string_cast.hpp>

using namespace vecs;

void CameraSystem::init() {
    // Set window resize event listener so we know to re-calculate our projection matrices
    EventManager::addListener(this, &CameraSystem::windowResize);

    // Create entity query so we can get our cameras
    cameras.filter.with(typeid(CameraComponent));
    cameras.filter.with(typeid(PushConstantComponent));
    cameras.onEntityAdded = EntityQuery::bind(this, &CameraSystem::onCameraAdded);
    world->addQuery(&cameras);
}

void CameraSystem::update() {
    for (uint32_t entity : cameras.entities) {
        CameraComponent* camera = world->getComponent<CameraComponent>(entity);
        
        // Re-calculate projection matrices if necessary
        if (camera->projDirty) {
            float aspectRatio = renderer->swapChainExtent.width /
                         (float)renderer->swapChainExtent.height;
            camera->projection = glm::perspective(glm::radians(45.0f), aspectRatio, 0.1f, 10.0f);
            // Flip projection's yy component because opengl has is flipped compared to vulkan and glm assumes opengl
            camera->projection[1][1] *= -1;
            camera->projDirty = false;
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
    PushConstantComponent* pushConstant = world->getComponent<PushConstantComponent>(entity);
    CameraComponent* camera = world->getComponent<CameraComponent>(entity);
    pushConstant->constants.resize(3);
    pushConstant->constants[0].data = &camera->model;
    pushConstant->constants[0].size = sizeof(glm::mat4);
    pushConstant->constants[0].stageFlags = VK_SHADER_STAGE_VERTEX_BIT;
    pushConstant->constants[1].data = &camera->view;
    pushConstant->constants[1].size = sizeof(glm::mat4);
    pushConstant->constants[1].stageFlags = VK_SHADER_STAGE_VERTEX_BIT;
    pushConstant->constants[2].data = &camera->projection;
    pushConstant->constants[2].size = sizeof(glm::mat4);
    pushConstant->constants[2].stageFlags = VK_SHADER_STAGE_VERTEX_BIT;
}
