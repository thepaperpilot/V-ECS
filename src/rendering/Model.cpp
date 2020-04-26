#include "Model.h"

#include "Renderer.h"
#include "SubRenderer.h"
#include "../engine/Debugger.h"

#include <cstdlib>
#include <cassert>
#include <iostream>

#define TINYOBJLOADER_IMPLEMENTATION
#include <tiny_obj_loader.h>

using namespace vecs;

Model::Model(SubRenderer* subrenderer, const char* filename) {
	this->device = subrenderer->device;
	this->vertexLayout = subrenderer->vertexLayout;

	init(subrenderer, filename);
}

Model::Model(SubRenderer* subrenderer, const char* filename, 
	VkShaderStageFlagBits shaderStage, std::vector<MaterialComponent> materialComponents) {
	this->device = subrenderer->device;
	this->vertexLayout = subrenderer->vertexLayout;
	this->materialShaderStage = shaderStage;
	this->materialComponents = materialComponents;
	hasMaterial = true;

	init(subrenderer, filename);
}

void Model::init(SubRenderer* subrenderer, const char* filename) {
	auto filepath = std::filesystem::path(filename).make_preferred();
	auto extension = filepath.extension();

	if (extension == ".obj") {
		loadObj(subrenderer->renderer->graphicsQueue, filepath);
	}
	else {
		Debugger::addLog(DEBUG_LEVEL_ERROR, "[MODEL] " + filepath.string() + " has unknown file format \"" + extension.string() + "\"");
		return;
	}

	subrenderer->models.emplace_back(this);
}

void Model::draw(VkCommandBuffer commandBuffer, VkPipelineLayout pipelineLayout) {
	VkBuffer vertexBuffers[] = { vertexBuffer };
	VkDeviceSize offsets[] = { 0 };
	vkCmdBindVertexBuffers(commandBuffer, 0, 1, vertexBuffers, offsets);
	vkCmdBindIndexBuffer(commandBuffer, indexBuffer, 0, VK_INDEX_TYPE_UINT32);
	vkCmdDrawIndexed(commandBuffer, indexCount, 1, 0, 0, 0);
}

void Model::cleanup() {
	device->cleanupBuffer(indexBuffer);
	device->cleanupBuffer(vertexBuffer);

	if (hasMaterial)
		device->cleanupBuffer(materialBuffer);
}

void Model::loadObj(VkQueue copyQueue, std::filesystem::path filepath) {
	tinyobj::attrib_t attrib;
	std::vector<tinyobj::shape_t> shapes;
	std::vector<tinyobj::material_t> materials;
	std::string warn, err;

	if (!tinyobj::LoadObj(&attrib, &shapes, &materials, &warn, &err, filepath.string().c_str(), filepath.parent_path().string().c_str())) {
		throw std::runtime_error(warn + err);
	}

	// Calculate min and max bounds
	for (uint32_t i = 0; i < attrib.vertices.size(); i += 3) {
		if (minBounds.x > attrib.vertices[i])
			minBounds.x = attrib.vertices[i];
		if (minBounds.y > attrib.vertices[i + 1])
			minBounds.y = attrib.vertices[i + 1];
		if (minBounds.z > attrib.vertices[i + 2])
			minBounds.z = attrib.vertices[i + 2];

		if (maxBounds.x < attrib.vertices[i])
			maxBounds.x = attrib.vertices[i];
		if (maxBounds.y < attrib.vertices[i + 1])
			maxBounds.y = attrib.vertices[i + 1];
		if (maxBounds.z < attrib.vertices[i + 2])
			maxBounds.z = attrib.vertices[i + 2];
	}

	if (hasMaterial) {
		std::vector<float> materialValues;
		uint16_t numFloats = 0;
		for (auto& mat : materials) {
			for (auto component : materialComponents) {
				switch (component) {
				case MATERIAL_COMPONENT_DIFFUSE:
					materialValues.emplace_back(mat.diffuse[0]);
					materialValues.emplace_back(mat.diffuse[1]);
					materialValues.emplace_back(mat.diffuse[2]);
					// We add an extra value for alignment, but we'll make it 1.0
					// so that it can be used as a default alpha value if the material
					// doesn't have a transparency component
					materialValues.emplace_back(1);
					break;
				}
			}
		}

		VkDeviceSize size = materialValues.size() * sizeof(float);
		materialBuffer = device->createBuffer(size,
			VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT);
		Buffer staging = device->createStagingBuffer(size);
		staging.copyTo(materialValues.data(), size);
		device->copyBuffer(&staging, &materialBuffer, copyQueue);

		materialBufferInfo.buffer = materialBuffer;
		materialBufferInfo.offset = 0;
		materialBufferInfo.range = size;
	}

	// Load vertices
	indexCount = 0;
	uint32_t vertexCount = 0;
	std::vector<void*> vertices;
	std::vector<uint32_t> indices;
	for (auto& shape : shapes) {
		size_t indexOffset = 0;
		size_t numFaces = shape.mesh.num_face_vertices.size();
		for (size_t face = 0; face < numFaces; face++) {
			auto mat = shape.mesh.material_ids[face];

			for (unsigned char i = 0; i < shape.mesh.num_face_vertices[face]; i++) {
				auto idx = shape.mesh.indices[indexOffset + i];

				// Allocate a block of memory the size of one vertex
				// It can have floats and integers so we'll use a void pointer,
				// as well as a second pointer to track where we're currently writing to
				void* vertex = malloc(vertexLayout->stride);
				void* head = vertex;

				// For each component, we'll write our data to our block, starting at head,
				// and increment head the appropriate amount so the next component is written
				// to the next part of the block
				for (auto component : vertexLayout->components) {
					switch (component.second) {
					case VERTEX_COMPONENT_POSITION:
						*((float*)head) = attrib.vertices[3 * idx.vertex_index];
						head = (float*)head + 1;
						*((float*)head) = attrib.vertices[3 * idx.vertex_index + 1];
						head = (float*)head + 1;
						*((float*)head) = attrib.vertices[3 * idx.vertex_index + 2];
						head = (float*)head + 1;
						break;
					case VERTEX_COMPONENT_NORMAL:
						*((float*)head) = attrib.vertices[3 * idx.normal_index];
						head = (float*)head + 1;
						*((float*)head) = attrib.vertices[3 * idx.normal_index + 1];
						head = (float*)head + 1;
						*((float*)head) = attrib.vertices[3 * idx.normal_index + 2];
						head = (float*)head + 1;
						break;
					case VERTEX_COMPONENT_UV:
						*((float*)head) = attrib.vertices[2 * idx.texcoord_index];
						head = (float*)head + 1;
						*((float*)head) = attrib.vertices[2 * idx.texcoord_index + 1];
						head = (float*)head + 1;
						break;
					case VERTEX_COMPONENT_MATERIAL_INDEX:
						*((int*)head) = mat;
						head = (int*)head + 1;
						break;
					default:
						// Ignore other cases because those are explicitly not set automatically by models
						// (it's probably a mistake for them to use a model while using a vertex component not listed above)
						Debugger::addLog(DEBUG_LEVEL_WARN, "[MODEL] " + filepath.string() + " loaded with a vertex layout using a custom vertex component");
						break;
					}
				}

				uint32_t index = vertexCount;
				bool uniqueVertex = true;
				// Look through our existing vertex for a duplicate
				// If none is found it'll use the initial index value,
				// which is where the next vertex would be on the list
				for (size_t i = 0; i < vertices.size(); i++) {
					if (memcmp(vertex, vertices[i], vertexLayout->stride) == 0) {
						// Its a match!
						index = i;
						uniqueVertex = false;
						break;
					}
				}

				if (uniqueVertex) {
					vertices.emplace_back(vertex);
					vertexCount++;
				} else {
					// Free our memory since it wasn't added to our list of vertices
					//free(vertex);
				}
				indices.emplace_back(index);
				indexCount++;
			}
			indexOffset += shape.mesh.num_face_vertices[face];
		}
	}

	// Create our buffers
	uint32_t vBufferSize = static_cast<uint32_t>(vertices.size()) * vertexLayout->stride;
	uint32_t iBufferSize = static_cast<uint32_t>(indices.size()) * sizeof(uint32_t);

	// Use staging buffer to move vertex and index buffer to device local memory
	Buffer vertexStaging = device->createStagingBuffer(vBufferSize);
	void* head = vertexStaging.map();
	for (void* vertex : vertices) {
		vertexStaging.copyTo(head, vertex, vertexLayout->stride);
		// Make sure to free our memory once its copied!
		free(vertex);
		head = (char*)head + vertexLayout->stride;
	}
	vertexStaging.unmap();

	Buffer indexStaging = device->createStagingBuffer(iBufferSize);
	indexStaging.copyTo(indices.data(), iBufferSize);

	// Initialize our device local target buffers
	vertexBuffer = device->createBuffer(vBufferSize, VK_BUFFER_USAGE_VERTEX_BUFFER_BIT);
	indexBuffer = device->createBuffer(iBufferSize, VK_BUFFER_USAGE_INDEX_BUFFER_BIT);

	// Copy from staging buffers
	VkCommandBuffer copyCmd = device->createCommandBuffer(VK_COMMAND_BUFFER_LEVEL_PRIMARY);

	VkBufferCopy copyRegion{};

	copyRegion.size = vBufferSize;
	vkCmdCopyBuffer(copyCmd, vertexStaging.buffer, vertexBuffer.buffer, 1, &copyRegion);

	copyRegion.size = iBufferSize;
	vkCmdCopyBuffer(copyCmd, indexStaging.buffer, indexBuffer.buffer, 1, &copyRegion);

	device->submitCommandBuffer(copyCmd, copyQueue);

	// Destroy staging resources
	device->cleanupBuffer(vertexStaging);
	device->cleanupBuffer(indexStaging);
}
