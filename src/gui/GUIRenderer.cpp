#include "GUIRenderer.h"

#include "GUIComponent.h"
#include "../rendering/Renderer.h"
#include "../ecs/World.h"

// The hunter package didn't include this and I don't know how to change that
// so I've included the file by hand in this directory, just modifying the #include statement
// They also have a slightly older version so no using ImGuiKey_KeyPadEnter or ImGuiMouseCursor_NotAllowed
//#include <imgui/examples/imgui_impl_glfw.h>
#include "imgui_impl_glfw.h"

#include <vulkan/vulkan.h>
#include <thread>

using namespace vecs;

GUIRenderer::GUIRenderer() {
	// Configure shaders
	shaders = {
		{ "resources/shaders/imgui.vert", VK_SHADER_STAGE_VERTEX_BIT },
		{ "resources/shaders/imgui.frag", VK_SHADER_STAGE_FRAGMENT_BIT }
	};

	alwaysDirty = true;
}

void GUIRenderer::init(World* world) {
	this->world = world;
	// Add entity query so we know what to draw
	guis.filter.with(typeid(GUIComponent));
	world->addQuery(&guis);
}

void GUIRenderer::init() {
	// Setup Dear ImGui context
	IMGUI_CHECKVERSION();
	ImGui::CreateContext();
	ImGuiIO& io = ImGui::GetIO();
	//io.ConfigFlags |= ImGuiConfigFlags_NavEnableKeyboard;     // Enable Keyboard Controls
	//io.ConfigFlags |= ImGuiConfigFlags_NavEnableGamepad;      // Enable Gamepad Controls

	// Setup Dear ImGui style
	ImGui::StyleColorsDark();
	//ImGui::StyleColorsClassic();

	// Setup GLFW + Vulkan implementation
	ImGui_ImplGlfw_InitForVulkan(renderer->window, true);
	// Note we don't setup the ImGui example Vulkan renderer because this subrenderer
	// already does everything it would, just slightly differently to support our ECS
	// stuff and maybe even slightly more optimized for our particular use cases
	// That said, I did use it as a reference for writing this file :)

	// Create our font texture from ImGUI's pixel data
	unsigned char* pixels;
	int width, height;
	io.Fonts->GetTexDataAsRGBA32(&pixels, &width, &height);
	fontTexture.init(device, renderer->graphicsQueue, pixels, width, height);
	io.Fonts->TexID = (ImTextureID)(intptr_t)fontTexture.image;

	// Create the font texture imageinfo we'll need for our descriptor writes
	imageInfo.imageLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
	imageInfo.imageView = fontTexture.view;
	imageInfo.sampler = fontTexture.sampler;
}

// TODO make this a system?
// No need, since it'd only ever be used with this renderer,
// and must exist if this renderer is in use
void GUIRenderer::preRender() {
	ImGui_ImplGlfw_NewFrame();
	ImGui::NewFrame();
	for (auto archetype : guis.matchingArchetypes) {
		for (auto component : *archetype->getComponentList(typeid(GUIComponent))) {
			GUIComponent* gui = static_cast<GUIComponent*>(component);

			// TODO do something with gui
		}
	}
	ImGui::ShowDemoWindow();
	static float f = 0.0f;
	static int counter = 0;

	ImGui::Begin("Hello, world!");                          // Create a window called "Hello, world!" and append into it.

	ImGui::Text("This is some useful text.");               // Display some text (you can use a format strings too)

	ImGui::SliderFloat("float", &f, 0.0f, 1.0f);            // Edit 1 float using a slider from 0.0f to 1.0f

	if (ImGui::Button("Button"))                            // Buttons return true when clicked (most widgets return true when edited/activated)
		counter++;
	ImGui::SameLine();
	ImGui::Text("counter = %d", counter);

	ImGui::Text("Application average %.3f ms/frame (%.1f FPS)", 1000.0f / ImGui::GetIO().Framerate, ImGui::GetIO().Framerate);
	ImGui::End();
	ImGui::Render();

	draw_data = ImGui::GetDrawData();

	// Resize our vertex and index buffers if necessary
	size_t vertexSize = draw_data->TotalVtxCount;
	size_t indexSize = draw_data->TotalIdxCount;

	if (vertexSize == 0) return;

	if (vertexBufferSize < vertexSize) {
		size_t size = vertexBufferSize || 1;
		// Double size until its equal to or greater than minSize
		while (size < vertexSize)
			size <<= 1;
		// recreate our vertex buffer with the new size
		vertexBuffer.cleanup();
		vertexBuffer = device->createBuffer(
			size * sizeof(ImDrawVert),
			VK_BUFFER_USAGE_VERTEX_BUFFER_BIT,
			VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT);
		vertexBufferSize = size;
	}
	if (indexBufferSize < indexSize) {
		size_t size = indexBufferSize || 1;
		// Double size until its equal to or greater than minSize
		while (size < indexSize)
			size <<= 1;
		// recreate our index buffer with the new size
		indexBuffer.cleanup();
		indexBuffer = device->createBuffer(
			size * sizeof(ImDrawIdx),
			VK_BUFFER_USAGE_INDEX_BUFFER_BIT,
			VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT);
		indexBufferSize = size;
	}

	// Fill our index and vertex buffers
	ImDrawVert* vtx_dst = (ImDrawVert*)vertexBuffer.map(vertexSize * sizeof(ImDrawVert), 0);
	ImDrawIdx* idx_dst = (ImDrawIdx*)indexBuffer.map(indexSize * sizeof(ImDrawIdx), 0);
	for (int n = 0; n < draw_data->CmdListsCount; n++) {
		const ImDrawList* cmd_list = draw_data->CmdLists[n];
		vertexBuffer.copyTo(vtx_dst, cmd_list->VtxBuffer.Data, cmd_list->VtxBuffer.Size * sizeof(ImDrawVert));
		indexBuffer.copyTo(idx_dst, cmd_list->IdxBuffer.Data, cmd_list->IdxBuffer.Size * sizeof(ImDrawIdx));
		vtx_dst += cmd_list->VtxBuffer.Size;
		idx_dst += cmd_list->IdxBuffer.Size;
	}
	// Flush our memory
	VkMappedMemoryRange range[2] = {};
	range[0].sType = VK_STRUCTURE_TYPE_MAPPED_MEMORY_RANGE;
	range[0].memory = vertexBuffer.memory;
	range[0].size = VK_WHOLE_SIZE;
	range[1].sType = VK_STRUCTURE_TYPE_MAPPED_MEMORY_RANGE;
	range[1].memory = indexBuffer.memory;
	range[1].size = VK_WHOLE_SIZE;
	if (vkFlushMappedMemoryRanges(device->logical, 2, range) != VK_SUCCESS) {
		throw std::runtime_error("failed to flush memory!");
	}
	vertexBuffer.unmap();
	indexBuffer.unmap();
}

void GUIRenderer::render(VkCommandBuffer buffer) {
	if (vertexBuffer.size == 0) return;

	// Submit our push constants
	float scale[2];
	scale[0] = 2.0f / draw_data->DisplaySize.x;
	scale[1] = 2.0f / draw_data->DisplaySize.y;
	float translate[2];
	translate[0] = -1.0f - draw_data->DisplayPos.x * scale[0];
	translate[1] = -1.0f - draw_data->DisplayPos.y * scale[1];

	vkCmdPushConstants(buffer, pipelineLayout, VK_SHADER_STAGE_VERTEX_BIT, sizeof(float) * 0, sizeof(float) * 2, scale);
	vkCmdPushConstants(buffer, pipelineLayout, VK_SHADER_STAGE_VERTEX_BIT, sizeof(float) * 2, sizeof(float) * 2, translate);

	// Bind our vertex and index buffers
	VkBuffer vertexBuffers[] = { vertexBuffer };
	VkDeviceSize offsets[] = { 0 };
	vkCmdBindVertexBuffers(buffer, 0, 1, vertexBuffers, offsets);
	vkCmdBindIndexBuffer(buffer, indexBuffer, 0, sizeof(ImDrawIdx) == 2 ? VK_INDEX_TYPE_UINT16 : VK_INDEX_TYPE_UINT32);

	// Will project scissor/clipping rectangles into framebuffer space
	ImVec2 clip_off = draw_data->DisplayPos;         // (0,0) unless using multi-viewports
	ImVec2 clip_scale = draw_data->FramebufferScale; // (1,1) unless using retina display which are often (2,2)

	// Render command lists
	// (Because we merged all buffers into a single one, we maintain our own offset into them)
	// imgui is slightly out of date, we can use ImDrawCmd->IdxOffset and ->VtxOffset once it gets updated
	int vtx_offset = 0;
	int idx_offset = 0;
	for (int n = 0; n < draw_data->CmdListsCount; n++) {
		const ImDrawList* cmd_list = draw_data->CmdLists[n];
		for (int cmd_i = 0; cmd_i < cmd_list->CmdBuffer.Size; cmd_i++) {
			const ImDrawCmd* pcmd = &cmd_list->CmdBuffer[cmd_i];
			if (pcmd->UserCallback != NULL) {
				// User callback, registered via ImDrawList::AddCallback()
				// (ImDrawCallback_ResetRenderState is a special callback value used by the user to request the renderer to reset render state.)
				if (pcmd->UserCallback == ImDrawCallback_ResetRenderState) {
					// TODO how to implement this?
				} else pcmd->UserCallback(cmd_list, pcmd);
			} else {
				// Project scissor/clipping rectangles into framebuffer space
				ImVec4 clip_rect;
				clip_rect.x = (pcmd->ClipRect.x - clip_off.x) * clip_scale.x;
				clip_rect.y = (pcmd->ClipRect.y - clip_off.y) * clip_scale.y;
				clip_rect.z = (pcmd->ClipRect.z - clip_off.x) * clip_scale.x;
				clip_rect.w = (pcmd->ClipRect.w - clip_off.y) * clip_scale.y;

				if (clip_rect.x < renderer->swapChainExtent.width && clip_rect.y < renderer->swapChainExtent.height &&
					clip_rect.z >= 0.0f && clip_rect.w >= 0.0f) {

					// Negative offsets are illegal for vkCmdSetScissor
					if (clip_rect.x < 0.0f)
						clip_rect.x = 0.0f;
					if (clip_rect.y < 0.0f)
						clip_rect.y = 0.0f;

					// Apply scissor/clipping rectangle
					VkRect2D scissor;
					scissor.offset.x = (int32_t)(clip_rect.x);
					scissor.offset.y = (int32_t)(clip_rect.y);
					scissor.extent.width = (uint32_t)(clip_rect.z - clip_rect.x);
					scissor.extent.height = (uint32_t)(clip_rect.w - clip_rect.y);
					vkCmdSetScissor(buffer, 0, 1, &scissor);

					// Draw
					vkCmdDrawIndexed(buffer, pcmd->ElemCount, 1, idx_offset, vtx_offset, 0);
				}
				idx_offset += pcmd->ElemCount;
			}
			vtx_offset += cmd_list->VtxBuffer.Size;
		}
	}
}

void GUIRenderer::preCleanup() {
	fontTexture.cleanup();
	vertexBuffer.cleanup();
	indexBuffer.cleanup();

	ImGui_ImplGlfw_Shutdown();
}

std::vector<VkDescriptorSetLayoutBinding> GUIRenderer::getDescriptorLayoutBindings() {
	VkDescriptorSetLayoutBinding samplerLayoutBinding = {};
	samplerLayoutBinding.binding = 0;
	samplerLayoutBinding.descriptorCount = 1;
	samplerLayoutBinding.descriptorType = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
	samplerLayoutBinding.pImmutableSamplers = &fontTexture.sampler;
	samplerLayoutBinding.stageFlags = VK_SHADER_STAGE_FRAGMENT_BIT;
	return { samplerLayoutBinding };
}

std::vector<VkWriteDescriptorSet> GUIRenderer::getDescriptorWrites(VkDescriptorSet descriptorSet) {
	VkWriteDescriptorSet descriptorWrite = {};
	descriptorWrite.sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
	descriptorWrite.dstSet = descriptorSet;
	descriptorWrite.dstBinding = 0;
	descriptorWrite.dstArrayElement = 0;
	descriptorWrite.descriptorType = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
	descriptorWrite.descriptorCount = 1;
	descriptorWrite.pImageInfo = &imageInfo;

	return { descriptorWrite };
}

VkVertexInputBindingDescription GUIRenderer::getBindingDescription() {
	VkVertexInputBindingDescription bindingDescription = {};
	bindingDescription.binding = 0;
	bindingDescription.stride = sizeof(ImDrawVert);
	bindingDescription.inputRate = VK_VERTEX_INPUT_RATE_VERTEX;

	return bindingDescription;
}

std::vector<VkVertexInputAttributeDescription> GUIRenderer::getAttributeDescriptions() {
	// Position
	VkVertexInputAttributeDescription position = {};
	position.binding = 0;
	position.location = 0;
	position.format = VK_FORMAT_R32G32_SFLOAT;
	position.offset = IM_OFFSETOF(ImDrawVert, pos);

	// UV
	VkVertexInputAttributeDescription uv = {};
	uv.binding = 0;
	uv.location = 1;
	uv.format = VK_FORMAT_R32G32_SFLOAT;
	uv.offset = IM_OFFSETOF(ImDrawVert, uv);

	// Col
	VkVertexInputAttributeDescription col = {};
	col.binding = 0;
	col.location = 2;
	col.format = VK_FORMAT_R8G8B8A8_UNORM;
	col.offset = IM_OFFSETOF(ImDrawVert, col);

	return { position, uv, col };
}

VkPipelineRasterizationStateCreateInfo GUIRenderer::getRasterizer() {
	VkPipelineRasterizationStateCreateInfo raster_info = {};
	raster_info.sType = VK_STRUCTURE_TYPE_PIPELINE_RASTERIZATION_STATE_CREATE_INFO;
	raster_info.polygonMode = VK_POLYGON_MODE_FILL;
	raster_info.cullMode = VK_CULL_MODE_NONE;
	raster_info.frontFace = VK_FRONT_FACE_COUNTER_CLOCKWISE;
	raster_info.lineWidth = 1.0f;

	return raster_info;
}

VkPipelineDepthStencilStateCreateInfo GUIRenderer::getDepthStencil() {
	VkPipelineDepthStencilStateCreateInfo depthStencil = {};
	depthStencil.sType = VK_STRUCTURE_TYPE_PIPELINE_DEPTH_STENCIL_STATE_CREATE_INFO;

	return depthStencil;
}

std::vector<VkPushConstantRange> GUIRenderer::getPushConstantRanges() {
	VkPushConstantRange pushConstantRange = {};
	pushConstantRange.stageFlags = VK_SHADER_STAGE_VERTEX_BIT;
	pushConstantRange.offset = 0;
	pushConstantRange.size = sizeof(float) * 4;

	return { pushConstantRange };
}
