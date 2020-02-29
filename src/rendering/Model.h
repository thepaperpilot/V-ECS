#pragma once

#include "Texture.h"
#include "../engine/Buffer.h"
#include "../engine/Device.h"

#include <vulkan/vulkan.h>

#include <glm/glm.hpp>

#include <filesystem>
#include <map>

namespace vecs {

	typedef enum VertexComponent {
		VERTEX_COMPONENT_POSITION,
		VERTEX_COMPONENT_NORMAL,
		VERTEX_COMPONENT_UV,
		VERTEX_COMPONENT_MATERIAL_INDEX
	} VertexComponent;

	// Maps binding locations to vertex components
	struct VertexLayout {
	public:
		std::map<uint8_t, uint8_t> components;
		uint16_t numFloats;
		VkVertexInputBindingDescription bindingDescription;
		std::vector<VkVertexInputAttributeDescription> attributeDescriptions;

		void init(std::map<uint8_t, uint8_t> components);
	};

	struct ModelPart {
		uint32_t indexStart;
		uint32_t indexCount;
		uint32_t matIndex;

		ModelPart(uint32_t indexStart, uint32_t indexCount, uint32_t matIndex) {
			this->indexStart = indexStart;
			this->indexCount = indexCount;
			this->matIndex = matIndex;
		}
	};

	// References:
	// https://pastebin.com/PZYVnJCd
	// https://www.reddit.com/r/vulkan/comments/826w5d/what_needs_to_be_done_in_order_to_load_obj_model/
	// https://github.com/SaschaWillems/Vulkan/blob/master/base/VulkanModel.hpp
	class Model {
	public:
		Buffer indexBuffer;
		Buffer vertexBuffer;
		uint32_t numMaterials;

		glm::vec3 minBounds = glm::vec3(FLT_MAX);
		glm::vec3 maxBounds = glm::vec3(-FLT_MAX);
		std::vector<Texture> textures;

		void init(Device* device, VkQueue copyQueue, const char* filename, VertexLayout* vertexLayout);

		void draw(VkCommandBuffer commandBuffer, VkPipelineLayout pipelineLayout, uint32_t pushConstantsOffset = 0, uint32_t materialOffset = 0);

		void cleanup();

	private:
		Device* device;

		VertexLayout* vertexLayout;
		std::vector<ModelPart> parts;
		bool hasMaterialIndices;

		void loadObj(VkQueue copyQueue, std::filesystem::path filepath);
	};
}
