#pragma once

#include "../ecs/World.h"
#include "../rendering/Vertex.h"

namespace vecs {

	// A component that stores the vertex and index buffers for
	// a piece of geometry
	struct MeshComponent : Component {
		std::vector<Vertex> vertices;
		std::vector<uint16_t> indices;

		VkBuffer vertexBuffer;
		VkDeviceMemory vertexBufferMemory;
		VkBuffer stagingVertexBuffer;
		VkDeviceMemory stagingVertexBufferMemory;
		size_t vertexBufferSize;

		VkBuffer indexBuffer;
		VkDeviceMemory indexBufferMemory;
		VkBuffer stagingIndexBuffer;
		VkDeviceMemory stagingIndexBufferMemory;
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

				// TODO Find any vertices that are no longer used in an update method if dirtyVertices = true

				dirtyVertices = true;
			}
		}

		void cleanup(VkDevice device) override {
			std::cout << "cleaning up mesh" << std::endl;
			vkDestroyBuffer(device, vertexBuffer, nullptr);
			// Also free its memory
			vkFreeMemory(device, vertexBufferMemory, nullptr);

			// Do the same for our staging buffer
			vkDestroyBuffer(device, stagingVertexBuffer, nullptr);
			vkFreeMemory(device, stagingVertexBufferMemory, nullptr);
		}
	};
}
