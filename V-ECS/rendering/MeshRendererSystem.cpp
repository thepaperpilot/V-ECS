#include "MeshRendererSystem.h"
#include "Renderer.h"
#include "../engine/Device.h"

#include <vulkan/vulkan.h>

using namespace vecs;

void MeshRendererSystem::onMeshAdded(uint32_t entity) {
    // Create our initial vertex buffer
    MeshComponent* mesh = world->getComponent<MeshComponent>(entity);
    mesh->createVertexBuffer(device, initialVertexBufferSize);
    fillVertexBuffer(mesh);
    mesh->createIndexBuffer(device, initialIndexBufferSize);
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
                    size >>= 1;
                // recreate our index buffer with the new size
                mesh->indexBuffer.cleanup();
                mesh->stagingIndexBuffer.cleanup();
                mesh->createIndexBuffer(device, size);
            }
            fillIndexBuffer(mesh);

            isDirty = true;
            mesh->dirtyVertices = false;
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
    unsigned long long verticesSize = sizeof(Vertex) * mesh->indices.size();
    mesh->stagingVertexBuffer.copyTo(mesh->vertices.data(), (VkDeviceSize)verticesSize);

    // Retrieve a command buffer we'll use to copy data from the staging to vertex buffer
    // TODO create an optimized command pool in our constructor after getting the graphics queue family index
    // Optimizing in this case means giving it the VK_COMMAND_POOL_CREATE_TRANSIENT_BIT flag
    device->createCommandBuffer(VK_COMMAND_BUFFER_LEVEL_PRIMARY);

    // Copy between the buffers
    VkBufferCopy copyRegion = {};
    copyRegion.size = verticesSize;
    device->copyBuffer(&mesh->stagingVertexBuffer, &mesh->vertexBuffer, renderer->graphicsQueue, &copyRegion);
}

void MeshRendererSystem::fillIndexBuffer(MeshComponent* mesh) {
    if (mesh->indices.size() == 0) return;

    // Map vertices data to our staging buffer
    // Note indicesSize will often be less than bufferSize
    unsigned long long indicesSize = sizeof(Vertex) * mesh->indices.size();
    mesh->stagingIndexBuffer.copyTo(mesh->indices.data(), (VkDeviceSize)indicesSize);

    // Retrieve a command buffer we'll use to copy data from the staging to index buffer
    // TODO create an optimized command pool in our constructor after getting the graphics queue family index
    // Optimizing in this case means giving it the VK_COMMAND_POOL_CREATE_TRANSIENT_BIT flag
    device->createCommandBuffer(VK_COMMAND_BUFFER_LEVEL_PRIMARY);

    // Copy between the buffers
    VkBufferCopy copyRegion = {};
    copyRegion.size = indicesSize;
    device->copyBuffer(&mesh->stagingIndexBuffer, &mesh->indexBuffer, renderer->graphicsQueue, &copyRegion);
}
