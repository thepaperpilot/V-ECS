#include "LuaRenderer.h"

#include "Renderer.h"
#include "Texture.h"
#include "../engine/Device.h"

#include "../lib/FrustumCull.h"

using namespace vecs;
using namespace luabridge;

struct vecs::LuaRendererLoaderHandle {
public:
	LuaRendererLoaderHandle(Device* device, Renderer* renderer, LuaRenderer* luaRenderer) {
		this->device = device;
		this->renderer = renderer;
		this->luaRenderer = luaRenderer;
	}

	LuaModelHandle* loadModel(const char* filename) {
		LuaModelHandle* modelHandle = new LuaModelHandle;
		modelHandle->model.init(device, renderer->graphicsQueue, filename, &luaRenderer->vertexLayout);
		luaRenderer->models.emplace_back(modelHandle);
		return modelHandle;
	}

	LuaTextureHandle* loadTexture(const char* filename) {
		LuaTextureHandle* textureHandle = new LuaTextureHandle;
		textureHandle->texture.init(device, renderer->graphicsQueue, filename);
		luaRenderer->textures.emplace_back(textureHandle);
		return textureHandle;
	}

private:
	Device* device;
	LuaRenderer* luaRenderer;
	Renderer* renderer;
};

struct vecs::LuaRendererRendererHandle {
public:
	LuaRendererRendererHandle(VkCommandBuffer commandBuffer, VkPipelineLayout pipelineLayout, LuaRenderer* luaRenderer) {
		this->commandBuffer = commandBuffer;
		this->pipelineLayout = pipelineLayout;
		this->luaRenderer = luaRenderer;
	}

	LuaMat4Handle* getMVP() {
		return new LuaMat4Handle(luaRenderer->getMVP());
	}

	LuaFrustumHandle* getFrustum() {
		return new LuaFrustumHandle(luaRenderer->getMVP());
	}

	void pushMat4(VkShaderStageFlagBits stageFlags, uint32_t offset, LuaMat4Handle mat4) {
		vkCmdPushConstants(commandBuffer, pipelineLayout, stageFlags, offset, sizeof(glm::mat4), &mat4.mat4);
	}

	void draw(LuaModelHandle model) {
		model.model.draw(commandBuffer, pipelineLayout);
	}

private:
	VkCommandBuffer commandBuffer;
	VkPipelineLayout pipelineLayout;
	LuaRenderer* luaRenderer;
};

namespace luabridge {
	template <>
	struct luabridge::Stack <VkShaderStageFlagBits> : EnumWrapper <VkShaderStageFlagBits> {};
	template <>
	struct luabridge::Stack <VertexComponent> : EnumWrapper <VertexComponent> {};
}

static int ShaderStageVertex = static_cast<int>(VK_SHADER_STAGE_VERTEX_BIT);
static int ShaderStageFragment = static_cast<int>(VK_SHADER_STAGE_FRAGMENT_BIT);

static int SizeMat4 = static_cast<int>(sizeof(glm::mat4));
static int SizeFloat = static_cast<int>(sizeof(float));

static int VertexComponentPosition = static_cast<int>(VERTEX_COMPONENT_POSITION);
static int VertexComponentNormal = static_cast<int>(VERTEX_COMPONENT_NORMAL);
static int VertexComponentUV = static_cast<int>(VERTEX_COMPONENT_UV);
static int VertexComponentMaterialIndex = static_cast<int>(VERTEX_COMPONENT_MATERIAL_INDEX);

lua_State* LuaRenderer::getState() {
	lua_State* L = vecs::getState();

	getGlobalNamespace(L)
		.beginNamespace("shaderStages")
			.addProperty("Vertex", &ShaderStageVertex, false)
			.addProperty("Fragment", &ShaderStageFragment, false)
		.endNamespace()
		.beginNamespace("sizes")
			.addProperty("mat4", &SizeMat4)
			.addProperty("float", &SizeFloat)
		.endNamespace()
		.beginNamespace("vertexComponents")
			.addProperty("Position", &VertexComponentPosition, false)
			.addProperty("Normal", &VertexComponentNormal, false)
			.addProperty("UV", &VertexComponentUV, false)
			.addProperty("MaterialIndex", &VertexComponentMaterialIndex, false)
		.endNamespace()
		.beginClass<LuaModelHandle>("modelHandle")
			.addFunction("cleanup", &LuaModelHandle::cleanup)
			.addProperty("minBounds", &LuaModelHandle::getMinBounds)
			.addProperty("maxBounds", &LuaModelHandle::getMaxBounds)
		.endClass()
		.beginClass<LuaTextureHandle>("textureHandle")
			.addFunction("cleanup", &LuaTextureHandle::cleanup)
		.endClass()
		.beginClass<LuaRendererLoaderHandle>("loaderHandle")
			.addFunction("loadModel", &LuaRendererLoaderHandle::loadModel)
			.addFunction("loadTexture", &LuaRendererLoaderHandle::loadTexture)
		.endClass()
		.beginClass<LuaRendererRendererHandle>("rendererHandle")
			.addFunction("getMVP", &LuaRendererRendererHandle::getMVP)
			.addFunction("getFrustum", &LuaRendererRendererHandle::getFrustum)
			.addFunction("pushMat4", &LuaRendererRendererHandle::pushMat4)
			.addFunction("draw", &LuaRendererRendererHandle::draw)
		.endClass();

	return L;
}

LuaRenderer::LuaRenderer(lua_State* L, const char* filename) : config(L) {
	// Load our file
	if (luaL_loadfile(L, filename) || lua_pcall(L, 0, 0, 0)) {
		std::cout << "Failed to load renderer " << filename << ":" << std::endl;
		std::cout << lua_tostring(L, -1) << std::endl;
		return;
	}

	// Fetch our config from the file
	config = config.getGlobal(L, "renderer");

	// Set our shader info
	// In the config its a table of filenames to shader stages
	// Here its a vector of structs, each with a filename and shader stage
	LuaRef shaders = config["shaders"];
	this->shaders.reserve(shaders.length());
	for (auto&& kvp : pairs(shaders)) {
		this->shaders.emplace_back(getString(kvp.first), kvp.second.cast<VkShaderStageFlagBits>());
	}

	// Set our push constants range, pulled directly from config
	pushConstantsRange = getInt(config["pushConstantsRange"]);

	// Set our vertex layout
	// In the config its a table of binding locations to vertex components
	// Here its a map of binding locations to vertex components
	LuaRef vertexComponents = config["vertexLayout"];
	std::map<uint8_t, uint8_t> components;
	for (auto&& kvp : pairs(vertexComponents)) {
		components[kvp.first] = kvp.second;
	}
	vertexLayout.init(components);

	alwaysDirty = true;
}

void LuaRenderer::init() {
	LuaRendererLoaderHandle loader(device, renderer, this);
	LuaRef init = config["init"];
	if (init.isFunction()) {
		init(&loader);
	}

	// Calculate number of textures from created objects
	numTextures = textures.size();
	imageInfos.reserve(numTextures);
	for (auto texture : textures)
		imageInfos.emplace_back(texture->texture.descriptor);
	for (auto model : models) {
		numTextures += model->model.textures.size();
		for (auto texture : model->model.textures)
			imageInfos.emplace_back(texture.descriptor);
	}
}

void LuaRenderer::render(VkCommandBuffer commandBuffer) {
	LuaRendererRendererHandle renderer(commandBuffer, pipelineLayout, this);
	LuaRef render = config["render"];
	if (render.isFunction()) {
		render(&renderer);
	}
}

void LuaRenderer::preCleanup() {
	LuaRef cleanup = config["cleanup"];
	if (cleanup.isFunction())
		cleanup();
}

glm::mat4 LuaRenderer::getMVP() {
	return *projection * *view * *model;
}

std::vector<VkDescriptorSetLayoutBinding> LuaRenderer::getDescriptorLayoutBindings() {
	VkDescriptorSetLayoutBinding samplerLayoutBinding = {};
	samplerLayoutBinding.binding = 1;
	samplerLayoutBinding.descriptorCount = numTextures;
	samplerLayoutBinding.descriptorType = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
	samplerLayoutBinding.pImmutableSamplers = nullptr;
	samplerLayoutBinding.stageFlags = VK_SHADER_STAGE_FRAGMENT_BIT;
	return { samplerLayoutBinding };
}

std::vector<VkWriteDescriptorSet> LuaRenderer::getDescriptorWrites(VkDescriptorSet descriptorSet) {
	if (numTextures == 0) return {};

	VkWriteDescriptorSet descriptorWrite = {};
	descriptorWrite.sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
	descriptorWrite.dstSet = descriptorSet;
	descriptorWrite.dstBinding = 1;
	descriptorWrite.dstArrayElement = 0;
	descriptorWrite.descriptorType = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
	descriptorWrite.descriptorCount = numTextures;
	descriptorWrite.pImageInfo = imageInfos.data();

	return { descriptorWrite };
}

std::vector<VkPushConstantRange> LuaRenderer::getPushConstantRanges() {
	VkPushConstantRange pushConstantRange = {};
	pushConstantRange.stageFlags = VK_SHADER_STAGE_VERTEX_BIT;
	pushConstantRange.offset = 0;
	pushConstantRange.size = pushConstantsRange;

	return { pushConstantRange };
}
