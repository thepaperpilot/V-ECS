#pragma once

#include <vulkan/vulkan.h>

#define SOL_ALL_SAFETIES_ON 1
#include <sol\sol.hpp>

#include <vector>
#include <map>

namespace vecs {
	typedef enum VertexComponent {
		VERTEX_COMPONENT_POSITION,
		VERTEX_COMPONENT_NORMAL,
		VERTEX_COMPONENT_UV,
		VERTEX_COMPONENT_MATERIAL_INDEX,
		// "Other" components that don't get set in models automatically,
		// for use in custom vertex+index buffers
		// TODO generate these for every corresponding VK_FORMAT?
		VERTEX_COMPONENT_R32,
		VERTEX_COMPONENT_R32G32,
		VERTEX_COMPONENT_R32G32B32,
		VERTEX_COMPONENT_R32G32B32A32,
		VERTEX_COMPONENT_R8_UNORM,
		VERTEX_COMPONENT_R8G8_UNORM,
		VERTEX_COMPONENT_R8G8B8_UNORM,
		VERTEX_COMPONENT_R8G8B8A8_UNORM,
		VERTEX_COMPONENT_R32_UINT,
		VERTEX_COMPONENT_R32G32_UINT,
		VERTEX_COMPONENT_R32G32B32_UINT,
		VERTEX_COMPONENT_R32G32B32A32_UINT,
		VERTEX_COMPONENT_R32_SINT,
		VERTEX_COMPONENT_R32G32_SINT,
		VERTEX_COMPONENT_R32G32B32_SINT,
		VERTEX_COMPONENT_R32G32B32A32_SINT
	} VertexComponent;

	// Maps binding locations to vertex components
	struct VertexLayout {
	public:
		uint16_t stride = 0;
		std::map<uint8_t, VertexComponent> components;
		VkVertexInputBindingDescription bindingDescription;
		std::vector<VkVertexInputAttributeDescription> attributeDescriptions;

		VertexLayout(sol::table config);
	};
}
