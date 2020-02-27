#pragma once

#include "Vertex.h"
#include "../../engine/Device.h"
#include "../../engine/Buffer.h"
#include "../../ecs/World.h"

#include <vector>

namespace vecs {

	// A component that stores the vertex and index buffers for
	// a piece of geometry
	struct MeshComponent : Component {
		std::vector<Vertex> vertices;
		std::vector<uint16_t> indices;

		Buffer vertexBuffer;
		Buffer stagingVertexBuffer;
		size_t vertexBufferSize = 0;

		Buffer indexBuffer;
		Buffer stagingIndexBuffer;
		size_t indexBufferSize = 0;

		// Used for frustum culling
		glm::vec3 minBounds;
		glm::vec3 maxBounds;

		bool dirtyVertices = true;

		void addFace(glm::vec3 p0, glm::vec3 p1, glm::vec4 uvs, bool isClockWise) {
			Vertex v0 = { p0, { uvs.s, uvs.t } };
			Vertex v1 = {};
			Vertex v2 = { p1, { uvs.p, uvs.q } };
			Vertex v3 = {};

			// Find the positions of v1 and v3 by checking which axis plane they're both on
			if (p0.x == p1.x) {
				v1.pos = glm::vec3(p0.x, p1.y, p0.z);
				v3.pos = glm::vec3(p0.x, p0.y, p1.z);
			} else if (p0.y == p1.y) {
				v1.pos = glm::vec3(p0.x, p0.y, p1.z);
				v3.pos = glm::vec3(p1.x, p0.y, p0.z);
			} else if (p0.z == p1.z) {
				v1.pos = glm::vec3(p0.x, p1.y, p0.z);
				v3.pos = glm::vec3(p1.x, p0.y, p0.z);
			} else return;

			v1.texCoord = { uvs.s, uvs.q };
			v3.texCoord = { uvs.p, uvs.t };

			uint16_t index = (uint16_t)vertices.size();
			vertices.insert(vertices.end(), { v0, v1, v2, v3 });
			if (isClockWise) {
				indices.insert(indices.end(), {
					// First Trianlge
					(uint16_t)(index + 0), (uint16_t)(index + 1), (uint16_t)(index + 2),
					// Second Triangle
					(uint16_t)(index + 2), (uint16_t)(index + 3), (uint16_t)(index + 0)
				});
			} else {
				indices.insert(indices.end(), {
					// First Trianlge
					(uint16_t)(index + 0), (uint16_t)(index + 2), (uint16_t)(index + 1),
					// Second Triangle
					(uint16_t)(index + 2), (uint16_t)(index + 0), (uint16_t)(index + 3)
				});
			}
		}

		void removeVertices(std::vector<Vertex> vertices) {
			// Calculate the indices of the vertices from start to end
			std::vector<uint16_t> vertexIndices;
			vertexIndices.resize(vertices.size());
			std::transform(vertices.begin(), vertices.end(), vertexIndices.begin(), [&vertices](Vertex vertex) {
				auto iter = std::find_if(vertices.begin(), vertices.end(), [&vertex](Vertex other) { return vertex.samePosition(other); });
				return std::distance(vertices.begin(), iter);
			});

			// Try to find sequence of our indices inside of our indices list
			auto iter = std::search(vertexIndices.begin(), vertexIndices.end(), indices.begin(), indices.end());
			if (iter != indices.end()) {
				// Remove the indices from the list
				indices.erase(iter, iter + vertexIndices.size());

				dirtyVertices = true;
			}
		}

		void removeFace(glm::vec3 p0, glm::vec3 p1, bool isClockWise) {
			Vertex v0 = { p0 };
			Vertex v1 = {};
			Vertex v2 = { p1 };
			Vertex v3 = {};

			if (isClockWise) {
				//removeVertices({ v0, v1, v2, v2, v3, v0 });
			} else {
				//removeVertices({ v0, v2, v1, v2, v0, v3 });
			}
		}

		// TODO remove coherent bits and flush memory manually?
		void createVertexBuffer(Device* device, size_t size) {
			vertexBufferSize = size;
			VkDeviceSize bufferSize = sizeof(Vertex) * size;

			// Create a staging buffer with the given size
			// This size should be more than we currently need,
			// so we don't need to reallocate another for awhile
			stagingVertexBuffer = device->createBuffer(
				bufferSize,
				VK_BUFFER_USAGE_TRANSFER_SRC_BIT,
				VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT);

			// Create a GPU-optimized buffer for the actual vertices
			vertexBuffer = device->createBuffer(
				bufferSize,
				VK_BUFFER_USAGE_TRANSFER_DST_BIT | VK_BUFFER_USAGE_VERTEX_BUFFER_BIT,
				VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT);
		}

		void createIndexBuffer(Device* device, size_t size) {
			indexBufferSize = size;
			VkDeviceSize bufferSize = sizeof(uint16_t) * size;

			// Create a staging buffer with the given size
			// This size should be more than we currently need,
			// so we don't need to reallocate another for awhile
			stagingIndexBuffer = device->createBuffer(
				bufferSize,
				VK_BUFFER_USAGE_TRANSFER_SRC_BIT,
				VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT);

			// Create a GPU-optimized buffer for the actual indices
			indexBuffer = device->createBuffer(
				bufferSize,
				VK_BUFFER_USAGE_TRANSFER_DST_BIT | VK_BUFFER_USAGE_INDEX_BUFFER_BIT,
				VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT);
		}

		void cleanup(VkDevice* device) override {
			vertexBuffer.cleanup();
			stagingVertexBuffer.cleanup();
			indexBuffer.cleanup();
			stagingIndexBuffer.cleanup();
		}
	};
}
