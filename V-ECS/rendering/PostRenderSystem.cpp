#include "PostRenderSystem.h"
#include "RenderStateComponent.h"
#include "Renderer.h"
#include "../engine/Device.h"

#include <vulkan/vulkan.h>

using namespace vecs;

void PostRenderSystem::init() {
    // Create entity query so we can get our render state
    renderState.filter.with(typeid(RenderStateComponent));
    world->addQuery(&renderState);
}

void PostRenderSystem::update() {
    if (this->renderState.entities.size() == 0) return;

    RenderStateComponent* renderState =
        world->getComponent<RenderStateComponent>(*this->renderState.entities.begin());

    // Create our info to submit an image to the buffers
    VkSubmitInfo submitInfo = {};
    submitInfo.sType = VK_STRUCTURE_TYPE_SUBMIT_INFO;
    // We need to set it up with a sempahore signal so it knows to wait until there's an image available
    VkSemaphore waitSemaphores[] = { renderState->imageAvailableSemaphores[renderState->currentFrame] };
    VkPipelineStageFlags waitStages[] = { VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT };
    submitInfo.waitSemaphoreCount = 1;
    submitInfo.pWaitSemaphores = waitSemaphores;
    submitInfo.pWaitDstStageMask = waitStages;
    // Tell our submission which command buffer to use to get the image
    submitInfo.commandBufferCount = 1;
    submitInfo.pCommandBuffers = &renderer->commandBuffers[renderState->imageIndex];
    // Tell our submission to send a signal on another semaphore so we know when the image has been submitted
    VkSemaphore signalSemaphores[] = { renderState->renderFinishedSemaphores[renderState->currentFrame] };
    submitInfo.signalSemaphoreCount = 1;
    submitInfo.pSignalSemaphores = signalSemaphores;

    // Submit our image to the graphic queue
    vkResetFences(*device, 1, &renderState->inFlightFences[renderState->currentFrame]);
    if (vkQueueSubmit(renderer->graphicsQueue, 1, &submitInfo, renderState->inFlightFences[renderState->currentFrame]) != VK_SUCCESS) {
        throw std::runtime_error("failed to submit draw command buffer!");
    }

    // Get ready to present the image to the screen
    VkPresentInfoKHR presentInfo = {};
    presentInfo.sType = VK_STRUCTURE_TYPE_PRESENT_INFO_KHR;
    // Tell it to wait for the image to be submitted using the semaphore we gave it earlier
    presentInfo.waitSemaphoreCount = 1;
    presentInfo.pWaitSemaphores = signalSemaphores;
    // Tell it what swap chain to send it to, and what index the image should go in
    VkSwapchainKHR swapChains[] = { renderer->swapChain };
    presentInfo.swapchainCount = 1;
    presentInfo.pSwapchains = swapChains;
    presentInfo.pImageIndices = &renderState->imageIndex;

    // Present the image to the screen
    VkResult result = vkQueuePresentKHR(renderer->presentQueue, &presentInfo);

    // Check if the image is out of date, so we can pre-emptively recreate the swap chain before next frame
    if (result == VK_ERROR_OUT_OF_DATE_KHR || result == VK_SUBOPTIMAL_KHR || renderState->framebufferResized) {
        renderState->framebufferResized = false;
        renderer->recreateSwapChain();
    } else if (result != VK_SUCCESS) {
        throw std::runtime_error("failed to present swap chain image!");
    }

    // Increment current frame so the next frame uses the next set of semaphores
    // and allow us to limit how many frames ahead we get
    renderState->currentFrame = (renderState->currentFrame + 1) % renderState->maxFramesInFlight;
}
