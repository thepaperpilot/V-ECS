#include "RenderingBindings.h"

#include "../ecs/World.h"
#include "../engine/Device.h"
#include "../rendering/Model.h"
#include "../rendering/SecondaryCommandBuffer.h"
#include "../rendering/SubRenderer.h"
#include "../rendering/VertexLayout.h"

#include <stb/stb_image.h>
#include "finders_interface.h" // rectpack2d

using namespace rectpack2D;

// Simple container for storing pixel data and the name of the file it came from
struct SubTexture {
	stbi_uc* pixels;
	std::string filename;
};

void vecs::RenderingBindings::setupState(sol::state& lua, Worker* worker, Device* device) {
	lua.new_enum("shaderStages",
		"Vertex", VK_SHADER_STAGE_VERTEX_BIT,
		"Fragment", VK_SHADER_STAGE_FRAGMENT_BIT
	);
	lua.new_enum("vertexComponents",
		"Position", VERTEX_COMPONENT_POSITION,
		"Normal", VERTEX_COMPONENT_NORMAL,
		"UV", VERTEX_COMPONENT_UV,
		"MaterialIndex", VERTEX_COMPONENT_MATERIAL_INDEX,
		// These are used for custom vertex components that don't get set automatically by models
		// Intended for renderers that create their own vertex+index buffers
		"R32", VERTEX_COMPONENT_R32,
		"R32G32", VERTEX_COMPONENT_R32G32,
		"R32G32B32", VERTEX_COMPONENT_R32G32B32,
		"R32G32B32A32", VERTEX_COMPONENT_R32G32B32A32,
		"R8_UNORM", VERTEX_COMPONENT_R8_UNORM,
		"R8G8_UNORM", VERTEX_COMPONENT_R8G8_UNORM,
		"R8G8B8_UNORM", VERTEX_COMPONENT_R8G8B8_UNORM,
		"R8G8B8A8_UNORM", VERTEX_COMPONENT_R8G8B8A8_UNORM,
		"R32_UINT", VERTEX_COMPONENT_R32_UINT,
		"R32G32_UINT", VERTEX_COMPONENT_R32G32_UINT,
		"R32G32B32_UINT", VERTEX_COMPONENT_R32G32B32_UINT,
		"R32G32B32A32_UINT", VERTEX_COMPONENT_R32G32B32A32_UINT,
		"R32_SINT", VERTEX_COMPONENT_R32_SINT,
		"R32G32_SINT", VERTEX_COMPONENT_R32G32_SINT,
		"R32G32B32_SINT", VERTEX_COMPONENT_R32G32B32_SINT,
		"R32G32B32A32_SINT", VERTEX_COMPONENT_R32G32B32A32_SINT
	);
	lua.new_enum("materialComponents",
		"Diffuse", MATERIAL_COMPONENT_DIFFUSE
	);
	lua.new_enum("cullModes",
		"Back", VK_CULL_MODE_BACK_BIT,
		"Front", VK_CULL_MODE_FRONT_BIT,
		"None", VK_CULL_MODE_NONE
	);
	lua.new_enum("bufferUsage",
		"VertexBuffer", VK_BUFFER_USAGE_VERTEX_BUFFER_BIT,
		"IndexBuffer", VK_BUFFER_USAGE_INDEX_BUFFER_BIT,
		"UniformBuffer", VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT
	);

	lua.new_usertype<SubRenderer>("renderer",
		"new", sol::no_constructor,
		"addTexture", [device](SubRenderer* renderer, Texture* texture) {
			VkDescriptorSet descriptorSet;

			VkDescriptorSetAllocateInfo allocateInfo = {};
			allocateInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_ALLOCATE_INFO;
			allocateInfo.descriptorPool = renderer->imguiDescriptorPool;
			allocateInfo.descriptorSetCount = 1;
			allocateInfo.pSetLayouts = &renderer->descriptorSetLayout;

			VK_CHECK_RESULT(vkAllocateDescriptorSets(*device, &allocateInfo, &descriptorSet));

			VkDescriptorImageInfo descriptorImage[1] = { texture->descriptor };
			VkWriteDescriptorSet writeDescriptor[1] = {};
			writeDescriptor[0].sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
			writeDescriptor[0].dstSet = descriptorSet;
			writeDescriptor[0].dstBinding = 1;
			writeDescriptor[0].descriptorCount = 1;
			writeDescriptor[0].descriptorType = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
			writeDescriptor[0].pImageInfo = descriptorImage;

			vkUpdateDescriptorSets(*device, 1, writeDescriptor, 0, nullptr);

			texture->imguiTexId = (ImTextureID)descriptorSet;
			renderer->textures.push_back(texture);
		},
		"pushConstantFloat", [](SubRenderer* renderer, SecondaryCommandBuffer commandBuffer, VkShaderStageFlags shaderStage, int offset, float constant) {
			vkCmdPushConstants(commandBuffer, renderer->pipelineLayout, shaderStage, offset, sizeof(float), &constant);
		},
		"pushConstantVec2", [](SubRenderer* renderer, SecondaryCommandBuffer commandBuffer, VkShaderStageFlags shaderStage, int offset, glm::vec2 constant) {
			vkCmdPushConstants(commandBuffer, renderer->pipelineLayout, shaderStage, offset, sizeof(glm::vec2), &constant);
		},
		"pushConstantVec3", [](SubRenderer* renderer, SecondaryCommandBuffer commandBuffer, VkShaderStageFlags shaderStage, int offset, glm::vec3 constant) {
			vkCmdPushConstants(commandBuffer, renderer->pipelineLayout, shaderStage, offset, sizeof(glm::vec3), &constant);
		},
		"pushConstantVec4", [](SubRenderer* renderer, SecondaryCommandBuffer commandBuffer, VkShaderStageFlags shaderStage, int offset, glm::vec4 constant) {
			vkCmdPushConstants(commandBuffer, renderer->pipelineLayout, shaderStage, offset, sizeof(glm::vec4), &constant);
		},
		"pushConstantMat4", [](SubRenderer* renderer, SecondaryCommandBuffer commandBuffer, VkShaderStageFlags shaderStage, int offset, glm::mat4 constant) {
			vkCmdPushConstants(commandBuffer, renderer->pipelineLayout, shaderStage, offset, sizeof(glm::mat4), &constant);
		},
		"draw", [](SubRenderer* renderer, SecondaryCommandBuffer commandBuffer, Model& model) { model.draw(commandBuffer, renderer->pipelineLayout); },
		"drawVertices", [](SubRenderer* renderer, SecondaryCommandBuffer commandBuffer, Buffer* vertexBuffer, Buffer* indexBuffer, int indexCount) {
			VkBuffer vertexBuffers[] = { vertexBuffer->buffer };
			VkDeviceSize offsets[] = { 0 };
			vkCmdBindVertexBuffers(commandBuffer, 0, 1, vertexBuffers, offsets);
			vkCmdBindIndexBuffer(commandBuffer, indexBuffer->buffer, 0, VK_INDEX_TYPE_UINT32);
			vkCmdDrawIndexed(commandBuffer, indexCount, 1, 0, 0, 0);
		},
		"startRendering", [worker](SubRenderer* renderer) -> SecondaryCommandBuffer* { return renderer->startRendering(worker); },
		"finishRendering", [](SubRenderer* renderer, SecondaryCommandBuffer commandBuffer) { renderer->finishRendering(commandBuffer); },
		"createUBO", [device, worker](SubRenderer* renderer, VkShaderStageFlagBits shaderStage, VkDeviceSize size) -> Buffer {
			Buffer buffer = device->createBuffer(size, VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT);
			worker->getWorld()->addBuffer(buffer);
			if (shaderStage == VK_SHADER_STAGE_VERTEX_BIT)
				renderer->addVertexUniform(&buffer, size);
			else
				renderer->addFragmentUniform(&buffer, size);
			return buffer;
		}
	);
	lua.new_usertype<SecondaryCommandBuffer>("commandBuffer", sol::no_constructor);
	lua.new_usertype<Model>("model",
		sol::factories(
			[worker](SubRenderer* subrenderer, const char* filename) -> Model* { return new Model(subrenderer, worker, filename); },
			[worker](SubRenderer* subrenderer, const char* filename, VkShaderStageFlagBits shaderStage, std::vector<MaterialComponent> materialComponents) -> Model* { return new Model(subrenderer, worker, filename, shaderStage, materialComponents); }
		),
		"minBounds", &Model::minBounds,
		"maxBounds", &Model::maxBounds
	);
	lua.new_usertype<Texture>("texture",
		sol::factories(
			[worker](SubRenderer* subrenderer, const char* filename) -> Texture* { return new Texture(subrenderer, worker, filename); },
			[worker](SubRenderer* subrenderer, unsigned char* pixels, int width, int height) -> Texture* { return new Texture(subrenderer, worker, pixels, width, height); },
			[worker](SubRenderer* subrenderer, const char* filename, bool addToSubrenderer) -> Texture* { return new Texture(subrenderer, worker, filename, addToSubrenderer); },
			[worker](SubRenderer* subrenderer, unsigned char* pixels, int width, int height, bool addToSubrenderer) -> Texture* { return new Texture(subrenderer, worker, pixels, width, height, addToSubrenderer); }
		),
		"createStitched", [&lua, worker](std::vector<std::string> images) -> std::tuple<unsigned char*, int, int, sol::table> {
			using spaces_type = empty_spaces<false, default_empty_spaces>;
			using rect_type = output_rect_t<spaces_type>;

			uint8_t texIdx = 0;
			sol::table map = lua.create_table();
			std::vector<SubTexture> subtextures;
			std::vector<rect_type> rects;

			// Read pixels for each sub-texture and add their size to our rects list
			subtextures.resize(images.size());
			rects.resize(images.size());
			bool foundImage = false;
			for (auto image : images) {
				int texChannels, texWidth, texHeight;
				subtextures[texIdx].pixels = stbi_load(image.c_str(), &texWidth, &texHeight, &texChannels, STBI_rgb_alpha);
				subtextures[texIdx].filename = std::filesystem::path(image).filename().string();
				rects[texIdx] = { 0, 0, texWidth + 2, texHeight + 2 };
				if (subtextures[texIdx].pixels == nullptr) {
					Debugger::addLog(DEBUG_LEVEL_WARN, "Unable to read image \"" + image + "\"");
				} else {
					foundImage = true;
				}
				texIdx++;
			}

			// Find optimal packing for our textures
			const auto texSize = find_best_packing<spaces_type>(
				rects,
				make_finder_input(
					// TODO how to handle more textures than can fit in here
					// (and can the GPU handle that much?)
					32768,
					1,
					[](rect_type&) {
						return callback_result::CONTINUE_PACKING;
					},
					[](rect_type&) {
						// TODO what to do if we can't fit all the textures?
						return callback_result::ABORT_PACKING;
					},
					flipping_option::DISABLED
				)
			);

			// Calculate UVs for each texture and store them in our map,
			// and store their pixels in our stitched texture pixels array
			unsigned char* pixels = new unsigned char[texSize.w * texSize.h * 4];
			for (int i = 0; i < subtextures.size(); i++) {
				auto rect = rects[i];
				rect.x++;
				rect.y++;
				rect.w -= 2;
				rect.h -= 2;
				map[subtextures[i].filename] = lua.create_table_with(
					"p", rect.x / (float)texSize.w,
					"q", rect.y / (float)texSize.h,
					"s", (rect.x + rect.w) / (float)texSize.w,
					"t", (rect.y + rect.h) / (float)texSize.h,
					"width", rect.w,
					"height", rect.h
				);

				// If no images successfully loaded, we can't really set any pixel values
				if (!foundImage) continue;

				for (int y = 0; y < rect.h; y++) {
					int mainOffset = (rect.y + y) * (texSize.w * 4) + rect.x * 4;
					int subOffset = y * (rect.w * 4);
					for (int x = 0; x < rect.w * 4; x++) {
						pixels[mainOffset + x] = subtextures[i].pixels[subOffset + x];
					}
				}
				// Extend the last pixel on each side by one, to help with floating point errors
				for (int y = 0; y < rect.h; y++) {
					// Left side
					int mainOffset = (rect.y + y) * (texSize.w * 4) + (rect.x - 1) * 4;
					for (int x = 0; x < 4; x++)
						pixels[mainOffset + x] = subtextures[i].pixels[y * (rect.w * 4) + x];
					// Right side
					mainOffset = (rect.y + y) * (texSize.w * 4) + (rect.x + rect.w) * 4;
					for (int x = 0; x < 4; x++)
						pixels[mainOffset + x] = subtextures[i].pixels[(y + 1) * (rect.w * 4) - 4 + x];
				}
				// Top side
				int topMainOffset = (rect.y - 1) * (texSize.w * 4) + rect.x * 4;
				for (int x = 0; x < rect.w * 4; x++)
					pixels[topMainOffset + x] = subtextures[i].pixels[x];
				// Bottom side
				int botMainOffset = (rect.y + rect.h) * (texSize.w * 4) + rect.x * 4;
				for (int x = 0; x < rect.w * 4; x++)
					pixels[botMainOffset + x] = subtextures[i].pixels[(rect.h - 1) * (rect.w * 4) + x];
			}

			// Create our texture and release the stbi pixel data
			return std::make_tuple(pixels, texSize.w, texSize.h, map);
		}
	);
	lua.new_usertype<unsigned char>("char", sol::no_constructor);
	lua.new_usertype<Buffer>("buffer",
		"new", sol::factories(
			[worker, device](VkBufferUsageFlags usageFlags, VkDeviceSize size) -> Buffer {
				assert(usageFlags != VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT && "Use renderer:createUBO!");
				Buffer buffer = device->createBuffer(size, usageFlags);
				worker->getWorld()->addBuffer(buffer);
				return buffer;
			}
		),
		"setDataInts", [worker, device](Buffer* buffer, std::vector<int> data) {
			Buffer staging = device->createStagingBuffer(data.size() * sizeof(int));
			staging.copyTo(data.data(), data.size() * sizeof(int));
			device->copyBuffer(&staging, buffer, worker);
			device->cleanupBuffer(staging);
		},
		"setDataFloats", [worker, device](Buffer* buffer, std::vector<float> data) {
			Buffer staging = device->createStagingBuffer(data.size() * sizeof(float));
			staging.copyTo(data.data(), data.size() * sizeof(float));
			device->copyBuffer(&staging, buffer, worker);
			device->cleanupBuffer(staging);
		}
	);
}
