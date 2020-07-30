#include "VertexLayout.h"

#include "../engine/Debugger.h"
#include "../lua/LuaVal.h"

using namespace vecs;

VertexLayout::VertexLayout(LuaVal config) {
    if (config.type != LUA_TYPE_TABLE) {
        Debugger::addLog(DEBUG_LEVEL_ERROR, "Vertex layout must be a table");
        return;
    }
    LuaVal::MapType* configMap = std::get<LuaVal::MapType*>(config.value);
    // Calculate numFloats and attribute descriptions
    attributeDescriptions.reserve(configMap->size());
    for (auto kvp : *configMap) {
        if (kvp.first.type != LUA_TYPE_NUMBER) {
            Debugger::addLog(DEBUG_LEVEL_WARN, "Vertex layout contained non-numeric key");
            continue;
        }
        if (kvp.second.type != LUA_TYPE_NUMBER) {
            Debugger::addLog(DEBUG_LEVEL_WARN, "Vertex layout contained non-numeric value");
            continue;
        }

        uint8_t location = std::get<double>(kvp.first.value);
        VertexComponent component = kvp.second.getEnum<VertexComponent>();
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
