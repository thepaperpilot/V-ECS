#pragma once

#include "../engine/Device.h"
#include "../engine/Buffer.h"
#include "../ecs/World.h"
#include "../rendering/Vertex.h"

namespace vecs {

	// A component that stores the vertex and index buffers for
	// a piece of geometry
	struct MeshComponent : Component {
		std::vector<Vertex> vertices;
		std::vector<uint16_t> indices;

		Buffer vertexBuffer;
		Buffer stagingVertexBuffer;
		size_t vertexBufferSize;

		Buffer indexBuffer;
		Buffer stagingIndexBuffer;
		size_t indexBufferSize;

		bool dirtyVertices = true;

		// Logic? In my component!
		// I think this ECS impurity is okay since its nearly a setter
		void addVertex(Vertex vertex) {
			// Search the list of current vertices for the vertex being added
			auto iter = std::find_if(vertices.begin(), vertices.end(), [&vertex](Vertex other) {
				// Determine if vertex and other are equivalent
				return vertex.equals(other);
				});

			if (iter == vertices.end()) {
				// If the vertex is unique, add it to our list of vertices
				indices.emplace_back(static_cast<uint16_t>(vertices.size()));
				vertices.emplace_back(vertex);
			} else {
				// Add the index of the vertex to our indices list
				indices.emplace_back(std::distance(vertices.begin(), iter));
			}

			dirtyVertices = true;
		}

		void removeVertices(std::vector<Vertex> vertices) {
			// Calculate the indices of the vertices from start to end
			std::vector<uint16_t> vertexIndices;
			vertexIndices.resize(vertices.size());
			std::transform(vertices.begin(), vertices.end(), vertexIndices.begin(), [&vertices](Vertex vertex) {
				auto iter = std::find_if(vertices.begin(), vertices.end(), [&vertex](Vertex other) {
					// Determine if vertex and other are equivalent
					return vertex.equals(other);
				});
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

		void createVertexBuffer(Device* device, size_t size) {
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
			VkDeviceSize bufferSize = sizeof(Vertex) * size;

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
