#include "Model.h"

#include <cassert>

#define TINYOBJLOADER_IMPLEMENTATION
#include <tiny_obj_loader.h>

using namespace vecs;

void Model::init(Device* device, VkQueue copyQueue, const char* filename, VertexLayout* vertexLayout) {
	this->device = device;
	this->vertexLayout = vertexLayout;

	hasMaterialIndices = false;
	for (auto component : vertexLayout->components) {
		if (component.second == VERTEX_COMPONENT_MATERIAL_INDEX) {
			hasMaterialIndices = true;
			break;
		}
	}

	auto filepath = std::filesystem::path(filename).make_preferred();
	auto extension = filepath.extension();

	if (extension == ".obj") {
		loadObj(copyQueue, filepath);
	} else {
		assert(0 && "Unknown model format");
	}
}

void Model::draw(VkCommandBuffer commandBuffer, VkPipelineLayout pipelineLayout, uint32_t pushConstantsOffset, uint32_t materialOffset) {
	VkBuffer vertexBuffers[] = { vertexBuffer };
	VkDeviceSize offsets[] = { 0 };
	vkCmdBindVertexBuffers(commandBuffer, 0, 1, vertexBuffers, offsets);
	vkCmdBindIndexBuffer(commandBuffer, indexBuffer, 0, VK_INDEX_TYPE_UINT32);

	for (auto part : parts) {
		if (hasMaterialIndices) {
			uint32_t materialIndex = part.matIndex + materialOffset;
			vkCmdPushConstants(commandBuffer, pipelineLayout, VK_SHADER_STAGE_FRAGMENT_BIT, pushConstantsOffset, sizeof(int), &materialIndex);
		}
		vkCmdDrawIndexed(commandBuffer, part.indexCount, 1, part.indexStart, 0, 0);
	}
}

void Model::cleanup() {
	indexBuffer.cleanup();
	vertexBuffer.cleanup();
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

	for (auto& mat : materials) {
		// TODO material layout
	}

	// Load vertices
	uint32_t indexStart = 0;
	uint32_t indexCount = 0;
	int currentMat = -1;
	std::vector<float> vertices;
	std::vector<uint32_t> indices;
	for (auto& shape : shapes) {
		size_t indexOffset = 0;
		for (auto& face : shape.mesh.num_face_vertices) {
			auto mat = shape.mesh.material_ids[face];
			if (mat != currentMat) {
				parts.emplace_back(indexStart, indexCount, currentMat);
				currentMat = mat;
				indexStart += indexCount;
				indexCount = 0;
			}

			for (unsigned char i = 0; i < shape.mesh.num_face_vertices[face]; i++) {
				auto idx = shape.mesh.indices[indexOffset + i];

				std::vector<float> vertex;
				vertex.reserve(vertexLayout->numFloats);

				for (auto component : vertexLayout->components) {
					switch (component.second) {
					case VERTEX_COMPONENT_POSITION:
						vertex.emplace_back(attrib.vertices[3 * idx.vertex_index]);
						vertex.emplace_back(attrib.vertices[3 * idx.vertex_index + 1]);
						vertex.emplace_back(attrib.vertices[3 * idx.vertex_index + 2]);
						break;
					case VERTEX_COMPONENT_NORMAL:
						vertex.emplace_back(attrib.normals[3 * idx.normal_index]);
						vertex.emplace_back(attrib.normals[3 * idx.normal_index + 1]);
						vertex.emplace_back(attrib.normals[3 * idx.normal_index + 2]);
						break;
					case VERTEX_COMPONENT_UV:
						vertex.emplace_back(attrib.texcoords[2 * idx.texcoord_index]);
						vertex.emplace_back(attrib.texcoords[2 * idx.texcoord_index + 1]);
						break;
					case VERTEX_COMPONENT_MATERIAL_INDEX:
						vertex.emplace_back(mat);
						break;
					}
				}

				uint32_t index = vertices.size() / vertexLayout->numFloats;
				bool uniqueVertex = true;
				// Look through our existing vertex for a duplicate
				// If none is found it'll use the initial index value,
				// which is where the next vertex would be on the list
				for (uint32_t i = 0; i < vertices.size(); i += vertexLayout->numFloats) {
					// Check each value of this vertex
					bool isDifferent = false;
					for (uint16_t j = 0; j < vertexLayout->numFloats; j++)
						if (vertices[i + j] != vertex[j]) {
							isDifferent = true;
							continue;
						}
					if (isDifferent) continue;

					// Its a match!
					index = i / vertexLayout->numFloats;
					uniqueVertex = false;
					break;
				}

				if (uniqueVertex) {
					vertices.insert(vertices.end(), vertex.begin(), vertex.end());
				}
				indices.emplace_back(index);
				indexCount++;
			}
			indexOffset += shape.mesh.num_face_vertices[face];
		}
	}
	parts.emplace_back(indexStart, indexCount, currentMat);

	// Create our buffers
	uint32_t vBufferSize = static_cast<uint32_t>(vertices.size()) * sizeof(float);
	uint32_t iBufferSize = static_cast<uint32_t>(indices.size()) * sizeof(uint32_t);

	// Use staging buffer to move vertex and index buffer to device local memory
	Buffer vertexStaging = device->createBuffer(vBufferSize, VK_BUFFER_USAGE_TRANSFER_SRC_BIT,
		VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT);
	vertexStaging.copyTo(vertices.data(), vBufferSize);

	Buffer indexStaging = device->createBuffer(iBufferSize, VK_BUFFER_USAGE_TRANSFER_SRC_BIT,
		VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT);
	indexStaging.copyTo(indices.data(), iBufferSize);

	// Initialize our device local target buffers
	device->createBuffer(vBufferSize,
		VK_BUFFER_USAGE_VERTEX_BUFFER_BIT | VK_BUFFER_USAGE_TRANSFER_DST_BIT,
		VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT, &vertexBuffer);

	device->createBuffer(iBufferSize,
		VK_BUFFER_USAGE_INDEX_BUFFER_BIT | VK_BUFFER_USAGE_TRANSFER_DST_BIT,
		VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT, &indexBuffer);

	// Copy from staging buffers
	VkCommandBuffer copyCmd = device->createCommandBuffer(VK_COMMAND_BUFFER_LEVEL_PRIMARY);

	VkBufferCopy copyRegion{};

	copyRegion.size = vBufferSize;
	vkCmdCopyBuffer(copyCmd, vertexStaging.buffer, vertexBuffer.buffer, 1, &copyRegion);

	copyRegion.size = iBufferSize;
	vkCmdCopyBuffer(copyCmd, indexStaging.buffer, indexBuffer.buffer, 1, &copyRegion);

	device->submitCommandBuffer(copyCmd, copyQueue);

	// Destroy staging resources
	vertexStaging.cleanup();
	indexStaging.cleanup();
}

void VertexLayout::init(std::map<uint8_t, uint8_t> components) {
	this->components = components;

	// Calculate numFloats and attribute descriptions
	numFloats = 0;
	attributeDescriptions.reserve(components.size());
	for (auto component : components) {
		VkVertexInputAttributeDescription attribute = {};
		attribute.binding = 0;
		attribute.location = component.first;
		attribute.offset = numFloats * 4;

		switch (component.second) {
		case VERTEX_COMPONENT_POSITION:
			numFloats += 3;
			attribute.format = VK_FORMAT_R32G32B32_SFLOAT;
			break;
		case VERTEX_COMPONENT_NORMAL:
			numFloats += 3;
			attribute.format = VK_FORMAT_R32G32B32_SFLOAT;
			break;
		case VERTEX_COMPONENT_UV:
			numFloats += 2;
			attribute.format = VK_FORMAT_R32G32_SFLOAT;
			break;
		case VERTEX_COMPONENT_MATERIAL_INDEX:
			// Indices are also floats because abstracting our vertices this way
			// doesn't allow us to have integers mixed with arbitrary numbers of floats
			numFloats += 1;
			attribute.format = VK_FORMAT_R32_SFLOAT;
			break;
		}

		attributeDescriptions.push_back(attribute);
	}

	// Create binding description
	bindingDescription.binding = 0;
	bindingDescription.stride = numFloats * sizeof(float);
	bindingDescription.inputRate = VK_VERTEX_INPUT_RATE_VERTEX;
}
