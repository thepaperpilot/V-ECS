#include "MeshRendererSystem.h"
#include "VoxelRenderer.h"
#include "../../engine/Device.h"

#include <vulkan/vulkan.h>

using namespace vecs;

void MeshRendererSystem::onMeshAdded(uint32_t entity) {
    MeshComponent* mesh = world->getComponent<MeshComponent>(entity);

    size_t minSize = mesh->vertices.size();
    size_t size = initialVertexBufferSize;
    // Double size until its equal to or greater than minSize
    while (size < minSize)
        size >>= 1;
    mesh->createVertexBuffer(device, size);
    fillVertexBuffer(mesh);

    minSize = mesh->indices.size();
    size = initialIndexBufferSize;
    // Double size until its equal to or greater than minSize
    while (size < minSize)
        size >>= 1;
    mesh->createIndexBuffer(device, size);
    fillIndexBuffer(mesh);
}

void MeshRendererSystem::init() {
    // Create entity query so we can get all our geometry objects to render each frame
    meshes.filter.with(typeid(MeshComponent));
    meshes.onEntityAdded = EntityQuery::bind(this, &MeshRendererSystem::onMeshAdded);
    world->addQuery(&meshes);
}

void MeshRendererSystem::update() {
    // Check if we need to rebuild any vertex and index buffers
    for (const uint32_t entity : meshes.entities) {
        MeshComponent* mesh = world->getComponent<MeshComponent>(entity);
        if (mesh->dirtyVertices) {
            // Now that our vertices have been re-optimized,
            // we can recreate our vertex buffer
            size_t minSize = mesh->vertices.size();
            if (minSize > mesh->vertexBufferSize) {
                size_t size = mesh->vertexBufferSize;
                // Double size until its equal to or greater than minSize
                while (size < minSize)
                    size <<= 1;
                // recreate our vertex buffer with the new size
                mesh->vertexBuffer.cleanup();
                mesh->stagingVertexBuffer.cleanup();
                mesh->createVertexBuffer(device, size);
            }
            fillVertexBuffer(mesh);
            // And our index buffer
            minSize = mesh->indices.size();
            if (minSize > mesh->indexBufferSize) {
                size_t size = mesh->indexBufferSize;
                // Double size until its equal to or greater than minSize
                while (size < minSize)
                    size <<= 1;
                // recreate our index buffer with the new size
                mesh->indexBuffer.cleanup();
                mesh->stagingIndexBuffer.cleanup();
                mesh->createIndexBuffer(device, size);
            }
            fillIndexBuffer(mesh);

            mesh->dirtyVertices = false;
            voxelRenderer->markAllBuffersDirty();
        }
    }
}

void MeshRendererSystem::cleanup() {
    // Destroy any remaining entities
    for (uint32_t entity : meshes.entities) {
        world->getComponent<MeshComponent>(entity)->cleanup(&device->logical);
    }
}

void MeshRendererSystem::fillVertexBuffer(MeshComponent* mesh) {
    if (mesh->vertices.size() == 0) return;

    // Map vertices data to our staging buffer
    // Note verticesSize will often be less than bufferSize
    unsigned long long verticesSize = sizeof(Vertex) * mesh->vertices.size();
    mesh->stagingVertexBuffer.copyTo(mesh->vertices.data(), (VkDeviceSize)verticesSize);

    // Retrieve a command buffer we'll use to copy data from the staging to vertex buffer
    // TODO create an optimized command pool in our constructor after getting the graphics queue family index
    // Optimizing in this case means giving it the VK_COMMAND_POOL_CREATE_TRANSIENT_BIT flag
    device->createCommandBuffer(VK_COMMAND_BUFFER_LEVEL_PRIMARY);

    // Copy between the buffers
    VkBufferCopy copyRegion = {};
    copyRegion.size = verticesSize;
    device->copyBuffer(&mesh->stagingVertexBuffer, &mesh->vertexBuffer, voxelRenderer->renderer->graphicsQueue, &copyRegion);
}

void MeshRendererSystem::fillIndexBuffer(MeshComponent* mesh) {
    if (mesh->indices.size() == 0) return;

    // Map vertices data to our staging buffer
    // Note indicesSize will often be less than bufferSize
    unsigned long long indicesSize = sizeof(uint16_t) * mesh->indices.size();
    mesh->stagingIndexBuffer.copyTo(mesh->indices.data(), (VkDeviceSize)indicesSize);

    // Retrieve a command buffer we'll use to copy data from the staging to index buffer
    // TODO create an optimized command pool in our constructor after getting the graphics queue family index
    // Optimizing in this case means giving it the VK_COMMAND_POOL_CREATE_TRANSIENT_BIT flag
    device->createCommandBuffer(VK_COMMAND_BUFFER_LEVEL_PRIMARY);

    // Copy between the buffers
    VkBufferCopy copyRegion = {};
    copyRegion.size = indicesSize;
    device->copyBuffer(&mesh->stagingIndexBuffer, &mesh->indexBuffer, voxelRenderer->renderer->graphicsQueue, &copyRegion);
}
