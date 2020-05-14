#pragma once

#include "Texture.h"
#include "../engine/Buffer.h"
#include "../engine/Device.h"

#include <vulkan/vulkan.h>

#include <glm/glm.hpp>

#include <filesystem>
#include <map>

namespace vecs {

	typedef enum MaterialComponent {
		MATERIAL_COMPONENT_DIFFUSE
	} MaterialComponent;

	// Forward declarations
	class SubRenderer;
	struct VertexLayout;

	// References:
	// https://pastebin.com/PZYVnJCd
	// https://www.reddit.com/r/vulkan/comments/826w5d/what_needs_to_be_done_in_order_to_load_obj_model/
	// https://github.com/SaschaWillems/Vulkan/blob/master/base/VulkanModel.hpp
	class Model {
	public:
		glm::vec3 minBounds = glm::vec3(FLT_MAX);
		glm::vec3 maxBounds = glm::vec3(-FLT_MAX);
		std::vector<Texture> textures;

		bool hasMaterial = false;
		VkShaderStageFlagBits materialShaderStage;
		VkDescriptorBufferInfo materialBufferInfo;

		Model(SubRenderer* subrenderer, const char* filename);
		Model(SubRenderer* subrenderer, const char* filename, VkShaderStageFlagBits shaderStage, std::vector<MaterialComponent> materialComponents);

		void draw(VkCommandBuffer commandBuffer, VkPipelineLayout pipelineLayout);

		void cleanup();

	private:
		Device* device;
		VertexLayout* vertexLayout;
		std::vector<MaterialComponent> materialComponents;

		Buffer indexBuffer;
		Buffer vertexBuffer;
		uint32_t indexCount;
		Buffer materialBuffer;

		void init(SubRenderer* subrenderer, const char* filename);
		void loadObj(VkQueue copyQueue, VkCommandPool commandPool, std::filesystem::path filepath);
	};
}
