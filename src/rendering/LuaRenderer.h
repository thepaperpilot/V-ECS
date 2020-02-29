#pragma once

#include "SubRenderer.h"

#include "Model.h"

#include "lua.hpp"
#include "lauxlib.h"
#include "lualib.h"
#include <LuaBridge/LuaBridge.h>
#include "../util/LuaUtils.h"

#include <glm/glm.hpp>

using namespace luabridge;

namespace vecs {

	struct LuaModelHandle {
	public:
		Model model;

		void cleanup() {
			model.cleanup();
		}

		LuaVec3Handle* getMinBounds() const {
			return new LuaVec3Handle(model.minBounds);
		}

		LuaVec3Handle* getMaxBounds() const {
			return new LuaVec3Handle(model.maxBounds);
		}
	};

	struct LuaTextureHandle {
	public:
		Texture texture;

		void cleanup() {
			texture.cleanup();
		}
	};

	// Forward Declarations
	struct LuaRendererLoaderHandle;
	struct LuaRendererRendererHandle;

	class LuaRenderer : public SubRenderer {
		friend struct LuaRendererLoaderHandle;
		friend struct LuaRendererRendererHandle;
	public:
		static lua_State* getState();

		glm::mat4* model;
		glm::mat4* view;
		glm::mat4* projection;

		LuaRenderer(lua_State* L, const char* filename);

		void init() override;
		void render(VkCommandBuffer commandBuffer) override;
		void preCleanup() override;

	protected:
		virtual std::vector<VkDescriptorSetLayoutBinding> getDescriptorLayoutBindings() override;
		virtual std::vector<VkWriteDescriptorSet> getDescriptorWrites(VkDescriptorSet descriptorSet) override;
		virtual VkVertexInputBindingDescription getBindingDescription() override {
			return vertexLayout.bindingDescription;
		};
		virtual std::vector<VkVertexInputAttributeDescription> getAttributeDescriptions() override {
			return vertexLayout.attributeDescriptions;
		}

		virtual std::vector<VkPushConstantRange> getPushConstantRanges() override;

	private:
		LuaRef config;

		uint16_t pushConstantsRange;
		std::vector<LuaModelHandle*> models;
		std::vector<LuaTextureHandle*> textures;
		VertexLayout vertexLayout;

		uint32_t numTextures;
		std::vector<VkDescriptorImageInfo> imageInfos;

		glm::mat4 getMVP();
	};
}
