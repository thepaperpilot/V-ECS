#include "PostRenderSystem.h"
#include "../engine/Engine.h"
#include "../engine/WindowResizeEvent.h"
#include "../events/EventManager.h"

#include <vulkan/vulkan.h>
#include <numeric>

using namespace vecs;

void PostRenderSystem::refreshWindow(RefreshWindowEvent* event) {
    engine->renderer.recreateSwapChain();
    update();
}

void PostRenderSystem::windowResize(WindowResizeEvent* event) {
    framebufferResized = true;
}

void PostRenderSystem::onMeshAdded(uint32_t entity) {
    // Create our initial vertex buffer
    MeshComponent* mesh = world->getComponent<MeshComponent>(entity);
    createVertexBuffer(mesh, initialVertexBufferSize);
    fillVertexBuffer(mesh);
    createIndexBuffer(mesh, initialIndexBufferSize);
    fillIndexBuffer(mesh);
}

PostRenderSystem::PostRenderSystem(Engine* engine) {
    this->engine = engine;
}

void PostRenderSystem::init() {
    // Create semaphores and fences for asynchronous rendering
    imageAvailableSemaphores.resize(maxFramesInFlight);
    renderFinishedSemaphores.resize(maxFramesInFlight);
    inFlightFences.resize(maxFramesInFlight);
    imagesInFlight.resize(engine->renderer.swapChainImages.size(), VK_NULL_HANDLE);

    // Semaphores need an info struct but it doesn't actually contain any info lol
    VkSemaphoreCreateInfo semaphoreInfo = {};
    semaphoreInfo.sType = VK_STRUCTURE_TYPE_SEMAPHORE_CREATE_INFO;

    // Make sure our fence starts in the signaled state so it starts rendering
    VkFenceCreateInfo fenceInfo = {};
    fenceInfo.sType = VK_STRUCTURE_TYPE_FENCE_CREATE_INFO;
    fenceInfo.flags = VK_FENCE_CREATE_SIGNALED_BIT;

    for (size_t i = 0; i < maxFramesInFlight; i++) {
        if (vkCreateSemaphore(engine->device, &semaphoreInfo, nullptr, &imageAvailableSemaphores[i]) != VK_SUCCESS ||
            vkCreateSemaphore(engine->device, &semaphoreInfo, nullptr, &renderFinishedSemaphores[i]) != VK_SUCCESS ||
            vkCreateFence(engine->device, &fenceInfo, nullptr, &inFlightFences[i]) != VK_SUCCESS) {

            throw std::runtime_error("failed to create synchronization objects for a frame!");
        }
    }

    // Set window resize event listener so we know to recreate our swap chain
    EventManager::addListener(this, &PostRenderSystem::windowResize);

    // Set window refresh event listener so we can recreate our swap chain and redraw as the window is resized
    EventManager::addListener(this, &PostRenderSystem::refreshWindow);

    // Create entity query so we can get all our geometry objects to render each frame
    meshes.filter.with(typeid(MeshComponent));
    meshes.onEntityAdded = EntityQuery::bind(this, &PostRenderSystem::onMeshAdded);
    world->addQuery(&meshes);
}

void PostRenderSystem::update() {
    // Make sure we're not sending frames too quickly for them to be actually drawn
    // by using a fence that will pause our code if we ever get too many frames ahead
    vkWaitForFences(engine->device, 1, &inFlightFences[currentFrame], VK_TRUE, UINT64_MAX);

    // Acquire our next image index
    uint32_t imageIndex;
    VkResult result = vkAcquireNextImageKHR(engine->device, engine->renderer.swapChain, UINT64_MAX,
        imageAvailableSemaphores[currentFrame], VK_NULL_HANDLE, &imageIndex);

    // Check for out of date image
    if (result == VK_ERROR_OUT_OF_DATE_KHR) {
        // We need to recreate the swap chain
        engine->renderer.recreateSwapChain();
        // Give up rendering this frame
        return;
    } else if (result != VK_SUCCESS && result != VK_SUBOPTIMAL_KHR) {
        throw std::runtime_error("failed to acquire swap chain image!");
    }

    // Check if a previous frame is using this image (i.e. there is its fence to wait on)
    if (imagesInFlight[imageIndex] != VK_NULL_HANDLE) {
        vkWaitForFences(engine->device, 1, &imagesInFlight[imageIndex], VK_TRUE, UINT64_MAX);
    }
    // Mark the image as now being in use by this frame
    imagesInFlight[imageIndex] = inFlightFences[currentFrame];

    // Check if we need to rebuild any vertex and index buffers
    bool isDirty = false;
    for (const uint32_t entity : meshes.entities) {
        MeshComponent* mesh = world->getComponent<MeshComponent>(entity);
        if (mesh->dirtyVertices) {
            // First remove any vertices with no corresponding indices
            // We do that here as opposed to when we remove the indices
            // because it'll take about the same amount of to check
            // independent of the number of entities removed,
            // so its faster to do it once after all removals have occured
            // than a bunch of times after each removal
            // We'll find the unused vertices by creating a list of numbers from
            // 0 to N - 1, where N is the number of unique vertices we have
            // Effectively being a list of vertex indices
            // We'll remove each index in the indices list from this set,
            // and at the end all the remaining indices are for the vertices
            // no longer in use
            // TODO differentiate between dirty vertices and dirty indices
            std::set<uint32_t> unusedVertices;
            size_t numVertices = mesh->vertices.size();
            for (size_t i = 0; i < numVertices; i++)
                unusedVertices.emplace(i);
            for (const uint32_t index : mesh->indices)
                // Attempt to remove every vertex in an index
                unusedVertices.erase(index);

            // Next we're going to shift all indices as appropriate 
            size_t numIndices = mesh->indices.size();
            for (size_t i = 0; i < numIndices; i++) {
                // TODO this can probably be optimized
                mesh->indices[i] -= std::count_if(unusedVertices.begin(), unusedVertices.end(),
                    [&mesh, &i](uint32_t index) {
                        return index < mesh->indices[i];
                    });
            }

            // Of course, we'll have to actually remove the vertices
            for (auto index : unusedVertices) {
                mesh->vertices[index] = mesh->vertices.back();
                mesh->vertices.pop_back();
            }                

            // Now that our vertices have been re-optimized,
            // we can recreate our vertex buffer
            size_t minSize = mesh->vertices.size();
            if (minSize > mesh->vertexBufferSize) {
                size_t size = mesh->vertexBufferSize;
                // Double size until its equal to or greater than minSize
                while (size < minSize)
                    size >>= 1;
                // recreate our vertex buffer with the new size
                cleanupVertexBuffer(mesh);
                createVertexBuffer(mesh, size);
            }
            fillVertexBuffer(mesh);
            // And our index buffer
            minSize = mesh->indices.size();
            if (minSize > mesh->indexBufferSize) {
                size_t size = mesh->indexBufferSize;
                // Double size until its equal to or greater than minSize
                while (size < minSize)
                    size >>= 1;
                // recreate our vertex buffer with the new size
                cleanupIndexBuffer(mesh);
                createIndexBuffer(mesh, size);
            }
            fillIndexBuffer(mesh);

            isDirty = true;
            mesh->dirtyVertices = false;
        }
    }
    if (isDirty) {
        // At least one vertex and index buffer got updated, so we need to redraw our geometry
        engine->renderer.createCommandBuffers();
    }

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
    submitInfo.pCommandBuffers = &engine->renderer.commandBuffers[imageIndex];
    // Tell our submission to send a signal on another semaphore so we know when the image has been submitted
    VkSemaphore signalSemaphores[] = { renderFinishedSemaphores[currentFrame] };
    submitInfo.signalSemaphoreCount = 1;
    submitInfo.pSignalSemaphores = signalSemaphores;

    // Submit our image to the graphic queue
    vkResetFences(engine->device, 1, &inFlightFences[currentFrame]);
    if (vkQueueSubmit(engine->renderer.graphicsQueue, 1, &submitInfo, inFlightFences[currentFrame]) != VK_SUCCESS) {
        throw std::runtime_error("failed to submit draw command buffer!");
    }

    // Get ready to present the image to the screen
    VkPresentInfoKHR presentInfo = {};
    presentInfo.sType = VK_STRUCTURE_TYPE_PRESENT_INFO_KHR;
    // Tell it to wait for the image to be submitted using the semaphore we gave it earlier
    presentInfo.waitSemaphoreCount = 1;
    presentInfo.pWaitSemaphores = signalSemaphores;
    // Tell it what swap chain to send it to, and what index the image should go in
    VkSwapchainKHR swapChains[] = { engine->renderer.swapChain };
    presentInfo.swapchainCount = 1;
    presentInfo.pSwapchains = swapChains;
    presentInfo.pImageIndices = &imageIndex;

    // Present the image to the screen
    result = vkQueuePresentKHR(engine->renderer.presentQueue, &presentInfo);

    // Check if the image is out of date, so we can pre-emptively recreate the swap chain before next frame
    if (result == VK_ERROR_OUT_OF_DATE_KHR || result == VK_SUBOPTIMAL_KHR || framebufferResized) {
        framebufferResized = false;
        engine->renderer.recreateSwapChain();
    } else if (result != VK_SUCCESS) {
        throw std::runtime_error("failed to present swap chain image!");
    }

    // Increment current frame so the next frame uses the next set of semaphores
    // and allow us to limit how many frames ahead we get
    currentFrame = (currentFrame + 1) % maxFramesInFlight;
}

void PostRenderSystem::cleanup() {
    // Destroy our semaphores
    for (size_t i = 0; i < maxFramesInFlight; i++) {
        vkDestroySemaphore(engine->device, renderFinishedSemaphores[i], nullptr);
        vkDestroySemaphore(engine->device, imageAvailableSemaphores[i], nullptr);
        vkDestroyFence(engine->device, inFlightFences[i], nullptr);
    }

    // Destroy any remaining entities
    for (uint32_t entity : meshes.entities) {
        cleanupVertexBuffer(world->getComponent<MeshComponent>(entity));
        cleanupIndexBuffer(world->getComponent<MeshComponent>(entity));
    }
}

void PostRenderSystem::createBuffer(VkDeviceSize size, VkBufferUsageFlags usage,
    VkMemoryPropertyFlags properties, VkBuffer& buffer, VkDeviceMemory& bufferMemory) {
    // Create our new buffer with the given size
    VkBufferCreateInfo bufferInfo = {};
    bufferInfo.sType = VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO;
    bufferInfo.size = size;
    bufferInfo.usage = usage;
    bufferInfo.sharingMode = VK_SHARING_MODE_EXCLUSIVE;

    if (vkCreateBuffer(engine->device, &bufferInfo, nullptr, &buffer) != VK_SUCCESS) {
        throw std::runtime_error("failed to create vertex buffer!");
    }

    // Assign memory to our buffer
    // First define our memory requirements
    VkMemoryRequirements memRequirements;
    vkGetBufferMemoryRequirements(engine->device, buffer, &memRequirements);

    // Define our memory allocation request
    VkMemoryAllocateInfo allocInfo = {};
    allocInfo.sType = VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO;
    allocInfo.allocationSize = memRequirements.size;
    allocInfo.memoryTypeIndex = findMemoryType(memRequirements.memoryTypeBits, properties);

    // Allocate memory
    if (vkAllocateMemory(engine->device, &allocInfo, nullptr, &bufferMemory) != VK_SUCCESS) {
        throw std::runtime_error("failed to allocate vertex buffer memory!");
    }

    // Bind this memory to our new buffer
    vkBindBufferMemory(engine->device, buffer, bufferMemory, 0);
}

void PostRenderSystem::createVertexBuffer(MeshComponent* mesh, size_t size) {
    VkDeviceSize bufferSize = sizeof(Vertex) * size;

    // Create a staging buffer with the given size
    // This size should be more than we currently need,
    // so we don't need to reallocate another for awhile
    createBuffer(
        bufferSize,
        VK_BUFFER_USAGE_TRANSFER_SRC_BIT,
        VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT,
        mesh->stagingVertexBuffer,
        mesh->stagingVertexBufferMemory
    );

    // Create a GPU-optimized buffer for the actual vertices
    createBuffer(
        bufferSize,
        VK_BUFFER_USAGE_TRANSFER_DST_BIT | VK_BUFFER_USAGE_VERTEX_BUFFER_BIT,
        VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT,
        mesh->vertexBuffer,
        mesh->vertexBufferMemory
    );
}

void PostRenderSystem::createIndexBuffer(MeshComponent* mesh, size_t size) {
    VkDeviceSize bufferSize = sizeof(Vertex) * size;

    // Create a staging buffer with the given size
    // This size should be more than we currently need,
    // so we don't need to reallocate another for awhile
    createBuffer(
        bufferSize,
        VK_BUFFER_USAGE_TRANSFER_SRC_BIT,
        VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT,
        mesh->stagingIndexBuffer,
        mesh->stagingIndexBufferMemory
    );

    // Create a GPU-optimized buffer for the actual vertices
    createBuffer(
        bufferSize,
        VK_BUFFER_USAGE_TRANSFER_DST_BIT | VK_BUFFER_USAGE_INDEX_BUFFER_BIT,
        VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT,
        mesh->indexBuffer,
        mesh->indexBufferMemory
    );
}

uint32_t PostRenderSystem::findMemoryType(uint32_t typeFilter, VkMemoryPropertyFlags properties) {
    // Get our different types of memories on our physical device
    VkPhysicalDeviceMemoryProperties memProperties;
    vkGetPhysicalDeviceMemoryProperties(engine->physicalDevice, &memProperties);

    for (uint32_t i = 0; i < memProperties.memoryTypeCount; i++) {
        // Check if this type of memory supports our filter and has all the properties we need
        // TODO rank and choose best memory type (e.g. use VRAM before swap)
        if ((typeFilter & (1 << i)) && (memProperties.memoryTypes[i].propertyFlags & properties) == properties) {
            return i;
        }
    }

    // Throw error if we can't find any memory that suits our needs
    throw std::runtime_error("failed to find suitable memory type!");
}

void PostRenderSystem::fillVertexBuffer(MeshComponent* mesh) {
    if (mesh->vertices.size() == 0) return;

    void* data;
    // Map vertices data to our staging buffer
    // Note verticesSize will often be less than bufferSize
    unsigned long long verticesSize = sizeof(Vertex) * mesh->vertices.size();
    vkMapMemory(engine->device, mesh->stagingVertexBufferMemory, 0, verticesSize, 0, &data);
    memcpy(data, mesh->vertices.data(), (size_t)verticesSize);
    vkUnmapMemory(engine->device, mesh->stagingVertexBufferMemory);

    // Retrieve a command buffer we'll use to copy data from the staging to vertex buffer
    // TODO create an optimized command pool in our constructor after getting the graphics queue family index
    // Optimizing in this case means giving it the VK_COMMAND_POOL_CREATE_TRANSIENT_BIT flag
    VkCommandBufferAllocateInfo allocInfo = {};
    allocInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO;
    allocInfo.level = VK_COMMAND_BUFFER_LEVEL_PRIMARY;
    allocInfo.commandPool = engine->renderer.commandPool;
    allocInfo.commandBufferCount = 1;
    VkCommandBuffer commandBuffer;
    vkAllocateCommandBuffers(engine->device, &allocInfo, &commandBuffer);

    // Start command buffer
    VkCommandBufferBeginInfo beginInfo = {};
    beginInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO;
    beginInfo.flags = VK_COMMAND_BUFFER_USAGE_ONE_TIME_SUBMIT_BIT;
    vkBeginCommandBuffer(commandBuffer, &beginInfo);

    // Copy between the buffers
    VkBufferCopy copyRegion = {};
    copyRegion.size = verticesSize;
    vkCmdCopyBuffer(commandBuffer, mesh->stagingVertexBuffer, mesh->vertexBuffer, 1, &copyRegion);

    // End command buffer
    vkEndCommandBuffer(commandBuffer);

    // Submit our command buffer to the graphics queue
    VkSubmitInfo submitInfo = {};
    submitInfo.sType = VK_STRUCTURE_TYPE_SUBMIT_INFO;
    submitInfo.commandBufferCount = 1;
    submitInfo.pCommandBuffers = &commandBuffer;

    vkQueueSubmit(engine->renderer.graphicsQueue, 1, &submitInfo, VK_NULL_HANDLE);
    vkQueueWaitIdle(engine->renderer.graphicsQueue);

    // Destroy our command buffer
    vkFreeCommandBuffers(engine->device, engine->renderer.commandPool, 1, &commandBuffer);
}

void PostRenderSystem::fillIndexBuffer(MeshComponent* mesh) {
    if (mesh->indices.size() == 0) return;

    void* data;
    // Map vertices data to our staging buffer
    // Note verticesSize will often be less than bufferSize
    unsigned long long indicesSize = sizeof(Vertex) * mesh->indices.size();
    vkMapMemory(engine->device, mesh->stagingIndexBufferMemory, 0, indicesSize, 0, &data);
    memcpy(data, mesh->indices.data(), (size_t)indicesSize);
    vkUnmapMemory(engine->device, mesh->stagingIndexBufferMemory);

    // Retrieve a command buffer we'll use to copy data from the staging to vertex buffer
    // TODO create an optimized command pool in our constructor after getting the graphics queue family index
    // Optimizing in this case means giving it the VK_COMMAND_POOL_CREATE_TRANSIENT_BIT flag
    VkCommandBufferAllocateInfo allocInfo = {};
    allocInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO;
    allocInfo.level = VK_COMMAND_BUFFER_LEVEL_PRIMARY;
    allocInfo.commandPool = engine->renderer.commandPool;
    allocInfo.commandBufferCount = 1;
    VkCommandBuffer commandBuffer;
    vkAllocateCommandBuffers(engine->device, &allocInfo, &commandBuffer);

    // Start command buffer
    VkCommandBufferBeginInfo beginInfo = {};
    beginInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO;
    beginInfo.flags = VK_COMMAND_BUFFER_USAGE_ONE_TIME_SUBMIT_BIT;
    vkBeginCommandBuffer(commandBuffer, &beginInfo);

    // Copy between the buffers
    VkBufferCopy copyRegion = {};
    copyRegion.size = indicesSize;
    vkCmdCopyBuffer(commandBuffer, mesh->stagingIndexBuffer, mesh->indexBuffer, 1, &copyRegion);

    // End command buffer
    vkEndCommandBuffer(commandBuffer);

    // Submit our command buffer to the graphics queue
    VkSubmitInfo submitInfo = {};
    submitInfo.sType = VK_STRUCTURE_TYPE_SUBMIT_INFO;
    submitInfo.commandBufferCount = 1;
    submitInfo.pCommandBuffers = &commandBuffer;

    vkQueueSubmit(engine->renderer.graphicsQueue, 1, &submitInfo, VK_NULL_HANDLE);
    vkQueueWaitIdle(engine->renderer.graphicsQueue);

    // Destroy our command buffer
    vkFreeCommandBuffers(engine->device, engine->renderer.commandPool, 1, &commandBuffer);
}

void PostRenderSystem::cleanupVertexBuffer(MeshComponent* mesh) {
    vkDestroyBuffer(engine->device, mesh->vertexBuffer, nullptr);
    // Also free its memory
    vkFreeMemory(engine->device, mesh->vertexBufferMemory, nullptr);

    // Do the same for our staging buffer
    vkDestroyBuffer(engine->device, mesh->stagingVertexBuffer, nullptr);
    vkFreeMemory(engine->device, mesh->stagingVertexBufferMemory, nullptr);
}

void PostRenderSystem::cleanupIndexBuffer(MeshComponent* mesh) {
    vkDestroyBuffer(engine->device, mesh->indexBuffer, nullptr);
    // Also free its memory
    vkFreeMemory(engine->device, mesh->indexBufferMemory, nullptr);

    // Do the same for our staging buffer
    vkDestroyBuffer(engine->device, mesh->stagingIndexBuffer, nullptr);
    vkFreeMemory(engine->device, mesh->stagingIndexBufferMemory, nullptr);
}
