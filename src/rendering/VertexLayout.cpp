#include "VertexLayout.h"

using namespace vecs;

VertexLayout::VertexLayout(sol::table config) {
    // Calculate numFloats and attribute descriptions
    attributeDescriptions.reserve(config.size());
    for (auto kvp : config) {
        uint8_t location = kvp.first.as<uint8_t>();
        VertexComponent component = kvp.second.as<VertexComponent>();
        components.insert({ location, component });

        VkVertexInputAttributeDescription attribute = {};
        attribute.binding = 0;
        attribute.location = location;
        attribute.offset = stride;

        // TODO better way of handling this?
        switch (component) {
        case VERTEX_COMPONENT_R32:
            stride += 4;
            attribute.format = VK_FORMAT_R32_SFLOAT;
            break;
        case VERTEX_COMPONENT_UV:
        case VERTEX_COMPONENT_R32G32:
            stride += 8;
            attribute.format = VK_FORMAT_R32G32_SFLOAT;
            break;
        case VERTEX_COMPONENT_POSITION:
        case VERTEX_COMPONENT_NORMAL:
        case VERTEX_COMPONENT_R32G32B32:
            stride += 12;
            attribute.format = VK_FORMAT_R32G32B32_SFLOAT;
            break;
        case VERTEX_COMPONENT_R32G32B32A32:
            stride += 16;
            attribute.format = VK_FORMAT_R32G32B32A32_SFLOAT;
            break;
        case VERTEX_COMPONENT_R8_UNORM:
            stride += 1;
            attribute.format = VK_FORMAT_R8_UNORM;
            break;
        case VERTEX_COMPONENT_R8G8_UNORM:
            stride += 2;
            attribute.format = VK_FORMAT_R8G8_UNORM;
            break;
        case VERTEX_COMPONENT_R8G8B8_UNORM:
            stride += 3;
            attribute.format = VK_FORMAT_R8G8B8_UNORM;
            break;
        case VERTEX_COMPONENT_R8G8B8A8_UNORM:
            stride += 4;
            attribute.format = VK_FORMAT_R8G8B8A8_UNORM;
            break;
        case VERTEX_COMPONENT_R32_UINT:
            stride += 4;
            attribute.format = VK_FORMAT_R32_UINT;
            break;
        case VERTEX_COMPONENT_R32G32_UINT:
            stride += 8;
            attribute.format = VK_FORMAT_R32G32_UINT;
            break;
        case VERTEX_COMPONENT_R32G32B32_UINT:
            stride += 12;
            attribute.format = VK_FORMAT_R32G32B32_UINT;
            break;
        case VERTEX_COMPONENT_R32G32B32A32_UINT:
            stride += 16;
            attribute.format = VK_FORMAT_R32G32B32A32_UINT;
            break;
        case VERTEX_COMPONENT_MATERIAL_INDEX:
        case VERTEX_COMPONENT_R32_SINT:
            stride += 4;
            attribute.format = VK_FORMAT_R32_SINT;
            break;
        case VERTEX_COMPONENT_R32G32_SINT:
            stride += 8;
            attribute.format = VK_FORMAT_R32G32_SINT;
            break;
        case VERTEX_COMPONENT_R32G32B32_SINT:
            stride += 12;
            attribute.format = VK_FORMAT_R32G32B32_SINT;
            break;
        case VERTEX_COMPONENT_R32G32B32A32_SINT:
            stride += 16;
            attribute.format = VK_FORMAT_R32G32B32A32_SINT;
            break;
        }

        attributeDescriptions.push_back(attribute);
    }

    // Create binding description
    bindingDescription.binding = 0;
    bindingDescription.stride = stride;
    bindingDescription.inputRate = VK_VERTEX_INPUT_RATE_VERTEX;
}
