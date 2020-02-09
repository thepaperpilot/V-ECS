#include "PreRenderSystem.h"
#include "RenderStateComponent.h"
#include "Renderer.h"
#include "../engine/Device.h"
#include "../engine/GLFWEvents.h"
#include "../events/EventManager.h"

#include <vulkan/vulkan.h>

using namespace vecs;

void PreRenderSystem::onRenderStateAdded(uint32_t entity) {
    RenderStateComponent* renderState = world->getComponent<RenderStateComponent>(entity);

    // Create semaphores and fences for asynchronous rendering
    renderState->imageAvailableSemaphores.resize(renderState->maxFramesInFlight);
    renderState->renderFinishedSemaphores.resize(renderState->maxFramesInFlight);
    renderState->inFlightFences.resize(renderState->maxFramesInFlight);
    renderState->imagesInFlight.resize(renderer->swapChainImages.size(), VK_NULL_HANDLE);

    // Semaphores need an info struct but it doesn't actually contain any info lol
    VkSemaphoreCreateInfo semaphoreInfo = {};
    semaphoreInfo.sType = VK_STRUCTURE_TYPE_SEMAPHORE_CREATE_INFO;

    // Make sure our fence starts in the signaled state so it starts rendering
    VkFenceCreateInfo fenceInfo = {};
    fenceInfo.sType = VK_STRUCTURE_TYPE_FENCE_CREATE_INFO;
    fenceInfo.flags = VK_FENCE_CREATE_SIGNALED_BIT;

    for (size_t i = 0; i < renderState->maxFramesInFlight; i++) {
        if (vkCreateSemaphore(*device, &semaphoreInfo, nullptr, &renderState->imageAvailableSemaphores[i]) != VK_SUCCESS ||
            vkCreateSemaphore(*device, &semaphoreInfo, nullptr, &renderState->renderFinishedSemaphores[i]) != VK_SUCCESS ||
            vkCreateFence(*device, &fenceInfo, nullptr, &renderState->inFlightFences[i]) != VK_SUCCESS) {

            throw std::runtime_error("failed to create synchronization objects for a frame!");
        }
    }
}

void PreRenderSystem::refreshWindow(RefreshWindowEvent* event) {
    renderer->recreateSwapChain();
    world->update(0);
}

void PreRenderSystem::windowResize(WindowResizeEvent* event) {
    if (renderState.entities.size() > 0)
        world->getComponent<RenderStateComponent>(*renderState.entities.begin())->framebufferResized = true;
}

void PreRenderSystem::init() {
    // Set window resize event listener so we know to recreate our swap chain
    EventManager::addListener(this, &PreRenderSystem::windowResize);

    // Set window refresh event listener so we can recreate our swap chain and redraw as the window is resized
    EventManager::addListener(this, &PreRenderSystem::refreshWindow);

    // Create entity query so we can get our render state
    renderState.filter.with(typeid(RenderStateComponent));
    renderState.onEntityAdded = EntityQuery::bind(this, &PreRenderSystem::onRenderStateAdded);
    world->addQuery(&renderState);

    // Add our render state entity
    uint32_t entity = world->createEntity();
    world->addComponent(entity, new RenderStateComponent);
}

void PreRenderSystem::update() {
    if (this->renderState.entities.size() == 0) return;

    RenderStateComponent* renderState =
        world->getComponent<RenderStateComponent>(*this->renderState.entities.begin());

    // Make sure we're not sending frames too quickly for them to be actually drawn
    // by using a fence that will pause our code if we ever get too many frames ahead
    vkWaitForFences(*device, 1, &renderState->inFlightFences[renderState->currentFrame], VK_TRUE, UINT64_MAX);

    // Acquire our next image index
    VkResult result = vkAcquireNextImageKHR(*device, renderer->swapChain, UINT64_MAX,
        renderState->imageAvailableSemaphores[renderState->currentFrame], VK_NULL_HANDLE, &renderState->imageIndex);

    // Check for out of date image
    if (result == VK_ERROR_OUT_OF_DATE_KHR) {
        // We need to recreate the swap chain
        renderer->recreateSwapChain();
        // Give up rendering this frame
        return;
    }
    else if (result != VK_SUCCESS && result != VK_SUBOPTIMAL_KHR) {
        throw std::runtime_error("failed to acquire swap chain image!");
    }

    // Check if a previous frame is using this image (i.e. there is its fence to wait on)
    if (renderState->imagesInFlight[renderState->imageIndex] != VK_NULL_HANDLE) {
        vkWaitForFences(*device, 1, &renderState->imagesInFlight[renderState->imageIndex], VK_TRUE, UINT64_MAX);
    }
    // Mark the image as now being in use by this frame
    renderState->imagesInFlight[renderState->imageIndex] = renderState->inFlightFences[renderState->currentFrame];
}

void PreRenderSystem::cleanup() {
    // Destroy any remaining entities
    for (uint32_t entity : renderState.entities) {
        world->getComponent<RenderStateComponent>(entity)->cleanup(&device->logical);
    }
}
