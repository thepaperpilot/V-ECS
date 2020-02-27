#pragma once

#include <vulkan/vulkan.h>
#include <glm/glm.hpp>
#include <array>

namespace vecs {

    struct Vertex {
        glm::vec3 pos;
        glm::vec2 texCoord;

        bool operator==(const Vertex& other) const {
            return pos == other.pos && texCoord == other.texCoord;
        }

        bool samePosition(const Vertex& other) {
            return pos == other.pos;
        }

        static VkVertexInputBindingDescription getBindingDescription() {
            VkVertexInputBindingDescription bindingDescription = {};
            bindingDescription.binding = 0;
            bindingDescription.stride = sizeof(Vertex);
            bindingDescription.inputRate = VK_VERTEX_INPUT_RATE_VERTEX;

            return bindingDescription;
        }

        static std::vector<VkVertexInputAttributeDescription> getAttributeDescriptions() {
            // Position
            VkVertexInputAttributeDescription position = {};
            position.binding = 0;
            position.location = 0;
            position.format = VK_FORMAT_R32G32B32_SFLOAT;
            position.offset = offsetof(Vertex, pos);
            
            // UV
            VkVertexInputAttributeDescription uv = {};
            uv.binding = 0;
            uv.location = 1;
            uv.format = VK_FORMAT_R32G32_SFLOAT;
            uv.offset = offsetof(Vertex, texCoord);

            std::vector<VkVertexInputAttributeDescription> descriptions(2);
            descriptions[0] = position;
            descriptions[1] = uv;
            return descriptions;
        }
    };
}
