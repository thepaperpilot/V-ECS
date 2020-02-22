#include "Renderer.h"
#include "Texture.h"
#include "../engine/Engine.h"
#include "../engine/Device.h"
#include "../events/EventManager.h"
#include "../engine/GLFWEvents.h"
#include "../ecs/World.h"

#include <algorithm>
#include <array>

using namespace vecs;

void Renderer::init(Device* device, VkSurfaceKHR surface, GLFWwindow* window) {
    this->device = device;
    this->surface = surface;
    this->window = window;

    // Select various properties by checking for our preferences or choosing a fallback
    surfaceFormat = chooseSwapSurfaceFormat(device->swapChainSupport.formats);
    presentMode = chooseSwapPresentMode(device->swapChainSupport.presentModes);

    // Retrieve the queue handles for the first in each of our queue families
    vkGetDeviceQueue(*device, device->queueFamilyIndices.graphics.value(), 0, &graphicsQueue);
    vkGetDeviceQueue(*device, device->queueFamilyIndices.present.value(), 0, &presentQueue);

    // Initialize everything for our render pass
    createSwapChain();
    depthTexture.init(device, graphicsQueue, swapChainExtent);
    createImageViews();
    createRenderPass();
    createFramebuffers();
    commandBuffers = device->createCommandBuffers(VK_COMMAND_BUFFER_LEVEL_PRIMARY, imageCount);

    // Create semaphores and fences for asynchronous rendering
    imageAvailableSemaphores.resize(maxFramesInFlight);
    renderFinishedSemaphores.resize(maxFramesInFlight);
    inFlightFences.resize(maxFramesInFlight);
    imagesInFlight.resize(imageCount, VK_NULL_HANDLE);

    // Semaphores need an info struct but it doesn't actually contain any info lol
    VkSemaphoreCreateInfo semaphoreInfo = {};
    semaphoreInfo.sType = VK_STRUCTURE_TYPE_SEMAPHORE_CREATE_INFO;

    // Make sure our fence starts in the signaled state so it starts rendering
    VkFenceCreateInfo fenceInfo = {};
    fenceInfo.sType = VK_STRUCTURE_TYPE_FENCE_CREATE_INFO;
    fenceInfo.flags = VK_FENCE_CREATE_SIGNALED_BIT;

    for (uint32_t i = 0; i < maxFramesInFlight; i++) {
        if (vkCreateSemaphore(*device, &semaphoreInfo, nullptr, &imageAvailableSemaphores[i]) != VK_SUCCESS ||
            vkCreateSemaphore(*device, &semaphoreInfo, nullptr, &renderFinishedSemaphores[i]) != VK_SUCCESS ||
            vkCreateFence(*device, &fenceInfo, nullptr, &inFlightFences[i]) != VK_SUCCESS) {

            throw std::runtime_error("failed to create synchronization objects for a frame!");
        }
    }

    // Set window resize event listener so we know to recreate our swap chain
    EventManager::addListener(this, &Renderer::refreshWindow);
}

void Renderer::acquireImage() {
    // Make sure we're not sending frames too quickly for them to be actually drawn
    // by using a fence that will pause our code if we ever get too many frames ahead
    vkWaitForFences(*device, 1, &inFlightFences[currentFrame], VK_TRUE, UINT64_MAX);

    while (true) {
        // Acquire our next image index
        VkResult result = vkAcquireNextImageKHR(*device, swapChain, UINT64_MAX,
            imageAvailableSemaphores[currentFrame], VK_NULL_HANDLE, &imageIndex);

        // Check for out of date image
        if (result == VK_ERROR_OUT_OF_DATE_KHR || result == VK_SUBOPTIMAL_KHR) {
            refreshWindow(nullptr);
        } else if (result != VK_SUCCESS) {
            throw std::runtime_error("failed to acquire swap chain image!");
        } else {
            break;
        }
    }    

    // Check if a previous frame is using this image (i.e. there is its fence to wait on)
    if (imagesInFlight[imageIndex] != VK_NULL_HANDLE) {
        vkWaitForFences(*device, 1, &imagesInFlight[imageIndex], VK_TRUE, UINT64_MAX);
    }
    
    // Mark the image as now being in use by this frame
    imagesInFlight[imageIndex] = inFlightFences[currentFrame];
}

void Renderer::presentImage(std::vector<SubRenderer*>* subrenderers) {
    buildCommandBuffer(subrenderers);

    // Create our info to submit an image to the buffers
    VkSubmitInfo submitInfo = {};
    submitInfo.sType = VK_STRUCTURE_TYPE_SUBMIT_INFO;
    // We need to set it up with a sempahore signal so it knows to wait until there's an image available
    VkSemaphore waitSemaphores[] = { imageAvailableSemaphores[currentFrame] };
    VkPipelineStageFlags waitStages[] = { VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT };
    submitInfo.waitSemaphoreCount = 1;
    submitInfo.pWaitSemaphores = waitSemaphores;
    submitInfo.pWaitDstStageMask = waitStages;
    // Tell our submission which command buffer to use to get the image
    submitInfo.commandBufferCount = 1;
    submitInfo.pCommandBuffers = &commandBuffers[imageIndex];
    // Tell our submission to send a signal on another semaphore so we know when the image has been submitted
    VkSemaphore signalSemaphores[] = { renderFinishedSemaphores[currentFrame] };
    submitInfo.signalSemaphoreCount = 1;
    submitInfo.pSignalSemaphores = signalSemaphores;

    // Submit our image to the graphic queue
    vkResetFences(*device, 1, &inFlightFences[currentFrame]);
    if (vkQueueSubmit(graphicsQueue, 1, &submitInfo, inFlightFences[currentFrame]) != VK_SUCCESS) {
        throw std::runtime_error("failed to submit draw command buffer!");
    }

    // Get ready to present the image to the screen
    VkPresentInfoKHR presentInfo = {};
    presentInfo.sType = VK_STRUCTURE_TYPE_PRESENT_INFO_KHR;
    // Tell it to wait for the image to be submitted using the semaphore we gave it earlier
    presentInfo.waitSemaphoreCount = 1;
    presentInfo.pWaitSemaphores = signalSemaphores;
    // Tell it what swap chain to send it to, and what index the image should go in
    VkSwapchainKHR swapChains[] = { swapChain };
    presentInfo.swapchainCount = 1;
    presentInfo.pSwapchains = swapChains;
    presentInfo.pImageIndices = &imageIndex;

    // Present the image to the screen
    VkResult result = vkQueuePresentKHR(presentQueue, &presentInfo);

    // Check if the image is out of date, so we can pre-emptively recreate the swap chain before next frame
    if (result == VK_ERROR_OUT_OF_DATE_KHR || result == VK_SUBOPTIMAL_KHR) {
        refreshWindow(nullptr);
    } else if (result != VK_SUCCESS) {
        throw std::runtime_error("failed to present swap chain image!");
    }

    // Increment current frame so the next frame uses the next set of semaphores
    // and allow us to limit how many frames ahead we get
    currentFrame = (currentFrame + 1) % maxFramesInFlight;
}

void Renderer::cleanup() {
    for (uint32_t i = 0; i < imageCount; i++) {
        // Destroy our swap chain images
        vkDestroyImageView(*device, swapChainImageViews[i], nullptr);
        // Destroy our frame buffers
        vkDestroyFramebuffer(*device, swapChainFramebuffers[i], nullptr);
    }

    // Destroy our command buffers
    vkFreeCommandBuffers(*device, device->commandPool, static_cast<uint32_t>(commandBuffers.size()), commandBuffers.data());

    // Destroy our depth texture
    if (depthTexture.image)
        depthTexture.cleanup();

    // Destroy our render pass
    vkDestroyRenderPass(*device, renderPass, nullptr);

    // Destroy our swap chain
    vkDestroySwapchainKHR(*device, swapChain, nullptr);

    // Destroy our sync objects
    for (uint32_t i = 0; i < maxFramesInFlight; i++) {
        vkDestroySemaphore(*device, renderFinishedSemaphores[i], nullptr);
        vkDestroySemaphore(*device, imageAvailableSemaphores[i], nullptr);
        vkDestroyFence(*device, inFlightFences[i], nullptr);
    }
}

void Renderer::createRenderPass() {
    // Describe how our color buffer will be attached to our frame buffer
    VkAttachmentDescription colorAttachment = {};
    // Format should always be whatever you used for your swap chain
    colorAttachment.format = swapChainImageFormat;
    // Number of samples in our multisampling AA
    colorAttachment.samples = VK_SAMPLE_COUNT_1_BIT;
    // What to do with the data in our color buffer before and after rendering
    // We're going to clear it to black before rendering, and store the memory after
    // so we can see it on screen
    colorAttachment.loadOp = VK_ATTACHMENT_LOAD_OP_CLEAR;
    colorAttachment.storeOp = VK_ATTACHMENT_STORE_OP_STORE;
    // Same but with our stencil buffer
    // We're not using one, so we don't care what happens to the data
    colorAttachment.stencilLoadOp = VK_ATTACHMENT_LOAD_OP_DONT_CARE;
    colorAttachment.stencilStoreOp = VK_ATTACHMENT_STORE_OP_DONT_CARE;
    // We'll tell it we don't care about the initial layout,
    // and by the end it'll be ready for rendering with our swap chain
    colorAttachment.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;
    colorAttachment.finalLayout = VK_IMAGE_LAYOUT_PRESENT_SRC_KHR;

    // Make our color attachment reference
    VkAttachmentReference colorAttachmentRef = {};
    colorAttachmentRef.attachment = 0;
    colorAttachmentRef.layout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;

    // Create our depth attachment
    VkAttachmentDescription depthAttachment = {};
    depthAttachment.format = depthTexture.format;
    depthAttachment.samples = VK_SAMPLE_COUNT_1_BIT;
    depthAttachment.loadOp = VK_ATTACHMENT_LOAD_OP_CLEAR;
    depthAttachment.storeOp = VK_ATTACHMENT_STORE_OP_DONT_CARE;
    depthAttachment.stencilLoadOp = VK_ATTACHMENT_LOAD_OP_DONT_CARE;
    depthAttachment.stencilStoreOp = VK_ATTACHMENT_STORE_OP_DONT_CARE;
    depthAttachment.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;
    depthAttachment.finalLayout = VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL;

    // Make our depth attachment reference
    VkAttachmentReference depthAttachmentRef = {};
    depthAttachmentRef.attachment = 1;
    depthAttachmentRef.layout = VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL;

    // Make our subpass
    VkSubpassDescription subpass = {};
    subpass.pipelineBindPoint = VK_PIPELINE_BIND_POINT_GRAPHICS;
    subpass.colorAttachmentCount = 1;
    subpass.pColorAttachments = &colorAttachmentRef;
    subpass.pDepthStencilAttachment = &depthAttachmentRef;

    // Make our subpass dependency
    VkSubpassDependency dependency = {};
    dependency.srcSubpass = VK_SUBPASS_EXTERNAL;
    dependency.dstSubpass = 0;
    dependency.srcStageMask = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT;
    dependency.srcAccessMask = 0;
    dependency.dstStageMask = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT;
    dependency.dstAccessMask = VK_ACCESS_COLOR_ATTACHMENT_READ_BIT | VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT;

    // Create our render pass
    std::array<VkAttachmentDescription, 2> attachments = { colorAttachment, depthAttachment };
    VkRenderPassCreateInfo renderPassInfo = {};
    renderPassInfo.sType = VK_STRUCTURE_TYPE_RENDER_PASS_CREATE_INFO;
    // Add our attachments
    renderPassInfo.attachmentCount = static_cast<uint32_t>(attachments.size());
    renderPassInfo.pAttachments = attachments.data();
    // Add our subpass
    renderPassInfo.subpassCount = 1;
    renderPassInfo.pSubpasses = &subpass;
    // Add our subpass dependency
    renderPassInfo.dependencyCount = 1;
    renderPassInfo.pDependencies = &dependency;

    if (vkCreateRenderPass(*device, &renderPassInfo, nullptr, &renderPass) != VK_SUCCESS) {
        throw std::runtime_error("failed to create render pass!");
    }
}

void Renderer::refreshWindow(RefreshWindowEvent* ignored) {
    // If we were minimized, wait until that changes
    int width = 0, height = 0;
    glfwGetFramebufferSize(window, &width, &height);
    while (width == 0 || height == 0) {
        glfwGetFramebufferSize(window, &width, &height);
        glfwWaitEvents();
    }

    // Wait until all current buffers finish
    vkDeviceWaitIdle(*device);

    // Destroy our current resources
    if (depthTexture.image) depthTexture.cleanup();
    for (int i = swapChainFramebuffers.size() - 1; i >= 0; i--) {
        vkDestroyImageView(*device, swapChainImageViews[i], nullptr);
        vkDestroyFramebuffer(*device, swapChainFramebuffers[i], nullptr);
    }

    // Create new ones
    // createSwapChain may (rarely) result in a new number of swap images
    // If it returns true we'll need to reconstruct a bunch of other stuff to
    bool numImagesChanged = createSwapChain(&swapChain);
    // Other resources always need to be updated when the window size changes
    depthTexture.init(device, graphicsQueue, swapChainExtent);
    createImageViews();
    createFramebuffers();

    // Only update image views and primary command buffers if the number of swap images changed
    if (numImagesChanged) {
        // Command Buffers
        vkFreeCommandBuffers(*device, device->commandPool, static_cast<uint32_t>(commandBuffers.size()), commandBuffers.data());
        commandBuffers = device->createCommandBuffers(VK_COMMAND_BUFFER_LEVEL_PRIMARY, imageCount);

        // Sync objects
        imagesInFlight.resize(imageCount, VK_NULL_HANDLE);
    }

    // Tell sub-renderers the window size changed as well
    for (auto subrenderer : engine->world->subrenderers) {
        subrenderer->windowRefresh(numImagesChanged, imageCount);
    }
}

bool Renderer::createSwapChain(VkSwapchainKHR* oldSwapChain) {
    VkSurfaceCapabilitiesKHR capabilities;
    vkGetPhysicalDeviceSurfaceCapabilitiesKHR(device->physical, surface, &capabilities);
    VkExtent2D extent = chooseSwapExtent(capabilities);

    // Tell our queue how many images to contain
    // Start with 1 more than the minimum, so we can buffer at least slightly
    uint32_t imageCount = capabilities.minImageCount + 1;

    // If we have a non-zero maximum, set our imageCount to be no bigger than that
    if (capabilities.maxImageCount > 0 &&
        imageCount > capabilities.maxImageCount) {
        imageCount = capabilities.maxImageCount;
    }

    // Configure the data we need to create our swap chain
    VkSwapchainCreateInfoKHR createInfo = {};
    createInfo.sType = VK_STRUCTURE_TYPE_SWAPCHAIN_CREATE_INFO_KHR;
    createInfo.surface = surface;
    createInfo.minImageCount = imageCount;
    createInfo.imageFormat = surfaceFormat.format;
    createInfo.imageColorSpace = surfaceFormat.colorSpace;
    createInfo.imageExtent = extent;
    createInfo.imageArrayLayers = 1;
    createInfo.imageUsage = VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT;

    // Setup whether we need to share between two family queues
    // This will happen if our present queue and graphics queue are different
    uint32_t queueFamilyIndices[] = { device->queueFamilyIndices.graphics.value(), device->queueFamilyIndices.present.value() };
    if (device->queueFamilyIndices.graphics.value() != device->queueFamilyIndices.present.value()) {
        // TODO Handle explicit ownership transfers
        createInfo.imageSharingMode = VK_SHARING_MODE_CONCURRENT;
        createInfo.queueFamilyIndexCount = 2;
        createInfo.pQueueFamilyIndices = queueFamilyIndices;
    } else {
        createInfo.imageSharingMode = VK_SHARING_MODE_EXCLUSIVE;
    }

    createInfo.preTransform = capabilities.currentTransform;
    createInfo.compositeAlpha = VK_COMPOSITE_ALPHA_OPAQUE_BIT_KHR;
    createInfo.presentMode = presentMode;
    createInfo.clipped = VK_TRUE;
    createInfo.oldSwapchain = oldSwapChain == nullptr ? VK_NULL_HANDLE : *oldSwapChain;

    if (vkCreateSwapchainKHR(*device, &createInfo, nullptr, &swapChain) != VK_SUCCESS) {
        throw std::runtime_error("failed to create swap chain!");
    }

    vkGetSwapchainImagesKHR(*device, swapChain, &imageCount, nullptr);
    swapChainImages.resize(imageCount);
    vkGetSwapchainImagesKHR(*device, swapChain, &imageCount, swapChainImages.data());
    swapChainImageFormat = surfaceFormat.format;
    swapChainExtent = extent;

    // Return true if the number of swap images changed
    bool imageCountChanged = imageCount != this->imageCount;
    this->imageCount = imageCount;
    return imageCountChanged;
}

void Renderer::createFramebuffers() {
    swapChainFramebuffers.resize(imageCount);

    // Create a frame buffer for each image view
    for (uint32_t i = 0; i < imageCount; i++) {
        std::array<VkImageView, 2> attachments = {
            swapChainImageViews[i],
            depthTexture.view
        };

        // Specify all our frame buffer info
        VkFramebufferCreateInfo framebufferInfo = {};
        framebufferInfo.sType = VK_STRUCTURE_TYPE_FRAMEBUFFER_CREATE_INFO;
        framebufferInfo.renderPass = renderPass;
        framebufferInfo.attachmentCount = static_cast<uint32_t>(attachments.size());
        framebufferInfo.pAttachments = attachments.data();
        framebufferInfo.width = swapChainExtent.width;
        framebufferInfo.height = swapChainExtent.height;
        framebufferInfo.layers = 1;

        // Create the frame buffer!
        if (vkCreateFramebuffer(*device, &framebufferInfo, nullptr, &swapChainFramebuffers[i]) != VK_SUCCESS) {
            throw std::runtime_error("failed to create framebuffer!");
        }
    }
}

void Renderer::createImageViews() {
    swapChainImageViews.resize(imageCount);

    for (uint32_t i = 0; i < imageCount; i++) {
        Texture::createImageView(device, swapChainImages[i], swapChainImageFormat, &swapChainImageViews[i]);
    }
}

void Renderer::buildCommandBuffer(std::vector<SubRenderer*>* subrenderers) {
    // Describe our render pass
    VkRenderPassBeginInfo renderPassInfo = {};
    renderPassInfo.sType = VK_STRUCTURE_TYPE_RENDER_PASS_BEGIN_INFO;
    renderPassInfo.renderPass = renderPass;
    // Attach the frame buffer from the swap chain to this render pass
    renderPassInfo.framebuffer = swapChainFramebuffers[imageIndex];
    // Tell the render pass where and how big of a space to render
    // These should match the attachments' sizes
    renderPassInfo.renderArea.offset = { 0, 0 };
    renderPassInfo.renderArea.extent = swapChainExtent;
    // Tell the render pass what color to use to clear the screen (black)
    std::array<VkClearValue, 2> clearValues = {};
    clearValues[0].color = { 0.0f, 0.0f, 0.0f, 1.0f };
    clearValues[1].depthStencil = { 1.0f, 0 };
    renderPassInfo.clearValueCount = static_cast<uint32_t>(clearValues.size());
    renderPassInfo.pClearValues = clearValues.data();

    // Make sure all our secondary command buffers are up to date
    std::vector<VkCommandBuffer> secondaryBuffers(subrenderers->size());
    for (int i = subrenderers->size() - 1; i >= 0; i--) {
        if (subrenderers->at(i)->dirtyBuffers.count(imageIndex))
            subrenderers->at(i)->buildCommandBuffer(imageIndex);
        secondaryBuffers[i] = subrenderers->at(i)->commandBuffers[imageIndex];
    }

    // Begin recording our command buffer
    device->beginCommandBuffer(commandBuffers[imageIndex]);

    // Begin the render pass
    vkCmdBeginRenderPass(commandBuffers[imageIndex], &renderPassInfo, VK_SUBPASS_CONTENTS_SECONDARY_COMMAND_BUFFERS);

    // Run our secondary command buffers
    vkCmdExecuteCommands(commandBuffers[imageIndex], secondaryBuffers.size(), secondaryBuffers.data());

    // End the render pass
    vkCmdEndRenderPass(commandBuffers[imageIndex]);

    // End the command buffer
    if (vkEndCommandBuffer(commandBuffers[imageIndex]) != VK_SUCCESS) {
        throw std::runtime_error("failed to record command buffer!");
    }    
}

VkSurfaceFormatKHR Renderer::chooseSwapSurfaceFormat(const std::vector<VkSurfaceFormatKHR>& availableFormats) {
    // If its available, prefer SRGB color space with RBG format
    for (const auto& availableFormat : availableFormats) {
        if (availableFormat.format == VK_FORMAT_B8G8R8A8_UNORM &&
            availableFormat.colorSpace == VK_COLOR_SPACE_SRGB_NONLINEAR_KHR) {
            return availableFormat;
        }
    }

    // Otherwise, just select the first format specified
    return availableFormats[0];
}

VkPresentModeKHR Renderer::chooseSwapPresentMode(const std::vector<VkPresentModeKHR>& availablePresentModes) {
    // If its available, prefer the mailbox presenting mode
    for (const auto& availablePresentMode : availablePresentModes) {
        if (availablePresentMode == VK_PRESENT_MODE_MAILBOX_KHR) {
            return availablePresentMode;
        }
    }

    // fifo is always available, so default to that
    return VK_PRESENT_MODE_FIFO_KHR;
}

VkExtent2D Renderer::chooseSwapExtent(const VkSurfaceCapabilitiesKHR& capabilities) {
    if (capabilities.currentExtent.width != UINT32_MAX) {
        return capabilities.currentExtent;
    }
    else {
        int width, height;
        glfwGetFramebufferSize(window, &width, &height);

        VkExtent2D actualExtent = {
            static_cast<uint32_t>(width),
            static_cast<uint32_t>(height)
        };

        actualExtent.width = std::max(capabilities.minImageExtent.width, std::min(capabilities.maxImageExtent.width, actualExtent.width));
        actualExtent.height = std::max(capabilities.minImageExtent.height, std::min(capabilities.maxImageExtent.height, actualExtent.height));

        return actualExtent;
    }
}
