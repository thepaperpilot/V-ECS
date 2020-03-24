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

	LuaMaterialHandle* createMaterial(VkShaderStageFlagBits shaderStage, LuaRef components) {
		Material layout;
		layout.shaderStage = shaderStage;
		for (auto&& kvp : pairs(components)) {
			layout.components[kvp.first] = kvp.second;
		}

		return new LuaMaterialHandle { layout };
	}

	LuaModelHandle* loadModelWithMaterials(const char* filename, LuaMaterialHandle* materialLayout) {
		LuaModelHandle* modelHandle = new LuaModelHandle;
		modelHandle->model.init(device, renderer->graphicsQueue, filename, &luaRenderer->vertexLayout, &materialLayout->matLayout);
		luaRenderer->models.emplace_back(modelHandle);
		return modelHandle;
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

	LuaMat4Handle* getViewProjectionMatrix() {
		return new LuaMat4Handle(luaRenderer->camera->projection * luaRenderer->camera->getViewMatrix());
	}

	LuaMat4Handle* getProjectionMatrix() {
		return new LuaMat4Handle(luaRenderer->camera->projection);
	}

	LuaMat4Handle* getViewMatrix(LuaVec3Handle eye, LuaVec3Handle center) {
		return new LuaMat4Handle(glm::lookAt(eye.vec3, center.vec3, { 0, 1, 0 }));
	}

	LuaMat4Handle* getViewMatrixWithUp(LuaVec3Handle eye, LuaVec3Handle center, LuaVec3Handle up) {
		return new LuaMat4Handle(glm::lookAt(eye.vec3, center.vec3, up.vec3));
	}

	LuaVec3Handle* getCameraPosition() {
		return new LuaVec3Handle(luaRenderer->camera->position);
	}

	LuaVec3Handle* getCameraForward() {
		return new LuaVec3Handle(luaRenderer->camera->forwardDir);
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
static int SizeVec3 = static_cast<int>(sizeof(glm::vec3));
static int SizeFloat = static_cast<int>(sizeof(float));

static int VertexComponentPosition = static_cast<int>(VERTEX_COMPONENT_POSITION);
static int VertexComponentNormal = static_cast<int>(VERTEX_COMPONENT_NORMAL);
static int VertexComponentUV = static_cast<int>(VERTEX_COMPONENT_UV);
static int VertexComponentMaterialIndex = static_cast<int>(VERTEX_COMPONENT_MATERIAL_INDEX);

static int MaterialComponentDiffuse = static_cast<int>(MATERIAL_COMPONENT_DIFFUSE);

lua_State* LuaRenderer::getState() {
	lua_State* L = vecs::getState();

	// TODO documentation for people writing their own lua renderer scripts
	getGlobalNamespace(L)
		.beginNamespace("shaderStages")
			.addProperty("Vertex", &ShaderStageVertex, false)
			.addProperty("Fragment", &ShaderStageFragment, false)
		.endNamespace()
		.beginNamespace("sizes")
			.addProperty("mat4", &SizeMat4)
			.addProperty("vec3", &SizeVec3)
			.addProperty("float", &SizeFloat)
		.endNamespace()
		.beginNamespace("vertexComponents")
			.addProperty("Position", &VertexComponentPosition, false)
			.addProperty("Normal", &VertexComponentNormal, false)
			.addProperty("UV", &VertexComponentUV, false)
			.addProperty("MaterialIndex", &VertexComponentMaterialIndex, false)
		.endNamespace()
		.beginNamespace("materialComponents")
			.addProperty("Diffuse", &MaterialComponentDiffuse, false)
		.endNamespace()
		.beginClass<LuaMaterialHandle>("materialHandle")
		.endClass()
		.beginClass<LuaModelHandle>("modelHandle")
			.addFunction("cleanup", &LuaModelHandle::cleanup)
			.addProperty("minBounds", &LuaModelHandle::getMinBounds)
			.addProperty("maxBounds", &LuaModelHandle::getMaxBounds)
		.endClass()
		.beginClass<LuaTextureHandle>("textureHandle")
			.addFunction("cleanup", &LuaTextureHandle::cleanup)
		.endClass()
		.beginClass<LuaRendererLoaderHandle>("loaderHandle")
			.addFunction("createMaterialLayout", &LuaRendererLoaderHandle::createMaterial)
			.addFunction("loadModelWithMaterials", &LuaRendererLoaderHandle::loadModelWithMaterials)
			.addFunction("loadModel", &LuaRendererLoaderHandle::loadModel)
			.addFunction("loadTexture", &LuaRendererLoaderHandle::loadTexture)
		.endClass()
		.beginClass<LuaRendererRendererHandle>("rendererHandle")
			.addFunction("getViewProjectionMatrix", &LuaRendererRendererHandle::getViewProjectionMatrix)
			.addFunction("getProjectionMatrix", &LuaRendererRendererHandle::getProjectionMatrix)
			.addFunction("getViewMatrix", &LuaRendererRendererHandle::getViewMatrix)
			.addFunction("getViewMatrixWithUp", &LuaRendererRendererHandle::getViewMatrixWithUp)
			.addFunction("getCameraPosition", &LuaRendererRendererHandle::getCameraPosition)
			.addFunction("getCameraForward", &LuaRendererRendererHandle::getCameraForward)
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

	// Read whether or not we should perform depth tests
	performDepthTest = getBool(config["performDepthTest"], true);

	// Read our render priority (only relevant if ignoring depth test)
	priority = getInt(config["renderPriority"]);

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
		if (model->model.materialLayout != nullptr) {
			switch (model->model.materialLayout->shaderStage) {
			case VK_SHADER_STAGE_VERTEX_BIT:
				numVertexUniforms++;
				vertexUniformBufferInfos.emplace_back(model->model.materialLayout->bufferInfo);
				break;
			case VK_SHADER_STAGE_FRAGMENT_BIT:
				numFragmentUniforms++;
				fragmentUniformBufferInfos.emplace_back(model->model.materialLayout->bufferInfo);
				break;
			}
		}
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
	for (auto texture : textures)
		texture->cleanup();
	for (auto model : models)
		model->cleanup();
}

std::vector<VkDescriptorSetLayoutBinding> LuaRenderer::getDescriptorLayoutBindings() {
	std::vector<VkDescriptorSetLayoutBinding> bindings;

	if (numTextures > 0) {
		VkDescriptorSetLayoutBinding samplerLayoutBinding = {};
		samplerLayoutBinding.binding = 1;
		samplerLayoutBinding.descriptorCount = numTextures;
		samplerLayoutBinding.descriptorType = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
		samplerLayoutBinding.pImmutableSamplers = nullptr;
		samplerLayoutBinding.stageFlags = VK_SHADER_STAGE_FRAGMENT_BIT;

		bindings.emplace_back(samplerLayoutBinding);
	}

	if (numVertexUniforms > 0) {
		VkDescriptorSetLayoutBinding uniformLayoutBinding = {};
		uniformLayoutBinding.binding = 2;
		uniformLayoutBinding.descriptorCount = numVertexUniforms;
		uniformLayoutBinding.descriptorType = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
		uniformLayoutBinding.pImmutableSamplers = nullptr;
		uniformLayoutBinding.stageFlags = VK_SHADER_STAGE_VERTEX_BIT;

		bindings.emplace_back(uniformLayoutBinding);
	}

	if (numFragmentUniforms > 0) {
		VkDescriptorSetLayoutBinding uniformLayoutBinding = {};
		uniformLayoutBinding.binding = 3;
		uniformLayoutBinding.descriptorCount = numFragmentUniforms;
		uniformLayoutBinding.descriptorType = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
		uniformLayoutBinding.pImmutableSamplers = nullptr;
		uniformLayoutBinding.stageFlags = VK_SHADER_STAGE_FRAGMENT_BIT;

		bindings.emplace_back(uniformLayoutBinding);
	}

	return bindings;
}

std::vector<VkWriteDescriptorSet> LuaRenderer::getDescriptorWrites(VkDescriptorSet descriptorSet) {
	std::vector<VkWriteDescriptorSet> descriptors;

	if (numTextures > 0) {
		VkWriteDescriptorSet descriptorWrite = {};
		descriptorWrite.sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
		descriptorWrite.dstSet = descriptorSet;
		descriptorWrite.dstBinding = 1;
		descriptorWrite.dstArrayElement = 0;
		descriptorWrite.descriptorType = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
		descriptorWrite.descriptorCount = numTextures;
		descriptorWrite.pImageInfo = imageInfos.data();

		descriptors.emplace_back(descriptorWrite);
	}

	if (numVertexUniforms > 0) {
		VkWriteDescriptorSet descriptorWrite = {};
		descriptorWrite.sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
		descriptorWrite.dstSet = descriptorSet;
		descriptorWrite.dstBinding = 2;
		descriptorWrite.dstArrayElement = 0;
		descriptorWrite.descriptorType = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
		descriptorWrite.descriptorCount = numVertexUniforms;
		descriptorWrite.pBufferInfo = vertexUniformBufferInfos.data();

		descriptors.emplace_back(descriptorWrite);
	}

	if (numFragmentUniforms > 0) {
		VkWriteDescriptorSet descriptorWrite = {};
		descriptorWrite.sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
		descriptorWrite.dstSet = descriptorSet;
		descriptorWrite.dstBinding = 3;
		descriptorWrite.dstArrayElement = 0;
		descriptorWrite.descriptorType = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
		descriptorWrite.descriptorCount = numFragmentUniforms;
		descriptorWrite.pBufferInfo = fragmentUniformBufferInfos.data();

		descriptors.emplace_back(descriptorWrite);
	}

	return descriptors;
}

VkPipelineDepthStencilStateCreateInfo LuaRenderer::getDepthStencil() {
	// Describe the depth and stencil buffer
	// We read in a variable (default true) for whether or not to perform depth testing for this renderer
	VkPipelineDepthStencilStateCreateInfo depthStencil = {};
	depthStencil.sType = VK_STRUCTURE_TYPE_PIPELINE_DEPTH_STENCIL_STATE_CREATE_INFO;
	depthStencil.depthTestEnable = performDepthTest ? VK_TRUE : VK_FALSE;
	depthStencil.depthWriteEnable = performDepthTest ? VK_TRUE : VK_FALSE;
	depthStencil.depthCompareOp = VK_COMPARE_OP_LESS;
	depthStencil.depthBoundsTestEnable = VK_FALSE;
	depthStencil.stencilTestEnable = VK_FALSE;

	return depthStencil;
}

std::vector<VkPushConstantRange> LuaRenderer::getPushConstantRanges() {
	VkPushConstantRange pushConstantRange = {};
	pushConstantRange.stageFlags = VK_SHADER_STAGE_VERTEX_BIT;
	pushConstantRange.offset = 0;
	pushConstantRange.size = pushConstantsRange;

	return { pushConstantRange };
}
