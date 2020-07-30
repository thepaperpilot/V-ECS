#include "imguiBindings.h"

#include "LuaVal.h"
#include "RenderingBindings.h"
#include "../ecs/World.h"
#include "../engine/Debugger.h"
#include "../engine/Device.h"
#include "../engine/Engine.h"
#include "../jobs/Worker.h"
#include "../rendering/SecondaryCommandBuffer.h"
#include "../rendering/SubRenderer.h"
#include "../rendering/Texture.h"

#include <imgui.h>
#include <examples\imgui_impl_glfw.h>
#include <misc/cpp/imgui_stdlib.h>
#include <glm/glm.hpp>

using namespace vecs;
using namespace ImGui;

// We use a mutex so that different systems can add
// there own imgui windows without needing to add complex
// dependencies and forwardDependencies. 
// That said, setting up the dependencies can still prevent
// some threads being blocked by others
std::mutex imguiMutex;

struct LuaCallback {
	LuaCallback(LuaVal* self, sol::function callback) {
		this->self = self;
		this->callback = callback;
	}

	LuaVal* self;
	sol::function callback;
};

static int InputTextCallback(ImGuiInputTextCallbackData* data) {
	LuaCallback* cb = (LuaCallback*)data->UserData;
	auto result = cb->callback(cb->self, data);
	if (result.valid()) {
		if (result.get_type() == sol::type::string) {
			std::string str = result;
			data->DeleteChars(0, data->BufTextLen);
			data->InsertChars(0, str.c_str());
			data->CursorPos = data->BufTextLen;
		}
	}
	else {
		sol::error err = result;
		Debugger::addLog(DEBUG_LEVEL_ERROR, "[LUA] " + std::string(err.what()));
		return 0;
	}
	return 0;
}

void vecs::imguiBindings::setupState(sol::state& lua, Worker* worker, Engine* engine, Device* device) {
	lua.new_enum("windowFlags",
		"AlwaysAutoResize", ImGuiWindowFlags_AlwaysAutoResize,
		"AlwaysHorizontalScrollbar", ImGuiWindowFlags_AlwaysHorizontalScrollbar,
		"AlwaysUseWindowPadding", ImGuiWindowFlags_AlwaysUseWindowPadding,
		"AlwaysVerticalScrollbar", ImGuiWindowFlags_AlwaysVerticalScrollbar,
		"ChildMenu", ImGuiWindowFlags_ChildMenu,
		"ChildWindow", ImGuiWindowFlags_ChildWindow,
		"HorizontalScrollbar", ImGuiWindowFlags_HorizontalScrollbar,
		"MenuBar", ImGuiWindowFlags_MenuBar,
		"Modal", ImGuiWindowFlags_Modal,
		"NavFlattened", ImGuiWindowFlags_NavFlattened,
		"NoBackground", ImGuiWindowFlags_NoBackground,
		"NoBringToFrontOnFocus", ImGuiWindowFlags_NoBringToFrontOnFocus,
		"NoCollapse", ImGuiWindowFlags_NoCollapse,
		"NoDecoration", ImGuiWindowFlags_NoDecoration,
		"NoFocusOnAppearing", ImGuiWindowFlags_NoFocusOnAppearing,
		"NoInputs", ImGuiWindowFlags_NoInputs,
		"NoMouseInputs", ImGuiWindowFlags_NoMouseInputs,
		"NoMove", ImGuiWindowFlags_NoMove,
		"NoNav", ImGuiWindowFlags_NoNav,
		"NoNavFocus", ImGuiWindowFlags_NoNavFocus,
		"NoNavInputs", ImGuiWindowFlags_NoNavInputs,
		"NoResize", ImGuiWindowFlags_NoResize,
		"NoSavedSettings", ImGuiWindowFlags_NoSavedSettings,
		"NoScrollbar", ImGuiWindowFlags_NoScrollbar,
		"NoScrollWithMouse", ImGuiWindowFlags_NoScrollWithMouse,
		"NoTitleBar", ImGuiWindowFlags_NoTitleBar,
		"Popup", ImGuiWindowFlags_Popup,
		"Tooltip", ImGuiWindowFlags_Tooltip,
		"UnsavedDocument", ImGuiWindowFlags_UnsavedDocument
	);
	lua.new_enum("inputTextFlags",
		"AllowTabInput", ImGuiInputTextFlags_AllowTabInput,
		"AlwaysInsertMode", ImGuiInputTextFlags_AlwaysInsertMode,
		"AutoSelectAll", ImGuiInputTextFlags_AutoSelectAll,
		"CallbackAlways", ImGuiInputTextFlags_CallbackAlways,
		"CallbackCharFilter", ImGuiInputTextFlags_CallbackCharFilter,
		"CallbackCompletion", ImGuiInputTextFlags_CallbackCompletion,
		"CallbackHistory", ImGuiInputTextFlags_CallbackHistory,
		"CallbackResize", ImGuiInputTextFlags_CallbackResize,
		"CharsDecimal", ImGuiInputTextFlags_CharsDecimal,
		"CharsHexadecimal", ImGuiInputTextFlags_CharsHexadecimal,
		"CharsNoBlank", ImGuiInputTextFlags_CharsNoBlank,
		"CharsScientific", ImGuiInputTextFlags_CharsScientific,
		"CharsUppercase", ImGuiInputTextFlags_CharsUppercase,
		"CtrlEnterForNewLine", ImGuiInputTextFlags_CtrlEnterForNewLine,
		"EnterReturnsTrue", ImGuiInputTextFlags_EnterReturnsTrue,
		"Multiline", ImGuiInputTextFlags_Multiline,
		"NoHorizontalScroll", ImGuiInputTextFlags_NoHorizontalScroll,
		"NoMarkEdited", ImGuiInputTextFlags_NoMarkEdited,
		"NoUndoRedo", ImGuiInputTextFlags_NoUndoRedo,
		"Password", ImGuiInputTextFlags_Password,
		"ReadOnly", ImGuiInputTextFlags_ReadOnly
	);
	lua.new_enum("styleColors",
		"Border", ImGuiCol_Border,
		"BorderShadow", ImGuiCol_BorderShadow,
		"Button", ImGuiCol_Button,
		"ButtonActive", ImGuiCol_ButtonActive,
		"ButtonHovered", ImGuiCol_ButtonHovered,
		"CheckMark", ImGuiCol_CheckMark,
		"ChildBg", ImGuiCol_ChildBg,
		"DragDropTarget", ImGuiCol_DragDropTarget,
		"FrameBg", ImGuiCol_FrameBg,
		"FrameBgActive", ImGuiCol_FrameBgActive,
		"FrameBgHovered", ImGuiCol_FrameBgHovered,
		"Header", ImGuiCol_Header,
		"HeaderActive", ImGuiCol_HeaderActive,
		"HeaderHovered", ImGuiCol_HeaderHovered,
		"MenuBarBg", ImGuiCol_MenuBarBg,
		"ModalWindowDarkening", ImGuiCol_ModalWindowDarkening,
		"ModalWindowDimBg", ImGuiCol_ModalWindowDimBg,
		"NavHighlight", ImGuiCol_NavHighlight,
		"NavWindowingDimBg", ImGuiCol_NavWindowingDimBg,
		"NavWindowingHighlight", ImGuiCol_NavWindowingHighlight,
		"PlotHistogram", ImGuiCol_PlotHistogram,
		"PlotHistogramHovered", ImGuiCol_PlotHistogramHovered,
		"PlotLines", ImGuiCol_PlotLines,
		"PlotLinesHovered", ImGuiCol_PlotLinesHovered,
		"PopupBg", ImGuiCol_PopupBg,
		"ResizeGrip", ImGuiCol_ResizeGrip,
		"ResizeGripActive", ImGuiCol_ResizeGripActive,
		"ResizeGripHovered", ImGuiCol_ResizeGripHovered,
		"ScrollbarBg", ImGuiCol_ScrollbarBg,
		"ScrollbarGrab", ImGuiCol_ScrollbarGrab,
		"ScrollbarGrabActive", ImGuiCol_ScrollbarGrabActive,
		"ScrollbarGrabHovered", ImGuiCol_ScrollbarGrabHovered,
		"Separator", ImGuiCol_Separator,
		"SeparatorActive", ImGuiCol_SeparatorActive,
		"SeparatorHovered", ImGuiCol_SeparatorHovered,
		"SliderGrab", ImGuiCol_SliderGrab,
		"SliderGrabActive", ImGuiCol_SliderGrabActive,
		"Tab", ImGuiCol_Tab,
		"TabActive", ImGuiCol_TabActive,
		"TabHovered", ImGuiCol_TabHovered,
		"TabUnfocused", ImGuiCol_TabUnfocused,
		"TabUnfocusedActive", ImGuiCol_TabUnfocusedActive,
		"Text", ImGuiCol_Text,
		"TextDisabled", ImGuiCol_TextDisabled,
		"TextSelectedBg", ImGuiCol_TextSelectedBg,
		"TitleBg", ImGuiCol_TitleBg,
		"TitleBgActive", ImGuiCol_TitleBgActive,
		"TitleBgCollapsed", ImGuiCol_TitleBgCollapsed,
		"WindowBg", ImGuiCol_WindowBg
	);
	lua.new_enum("styleVars",
		"Alpha", ImGuiStyleVar_Alpha,
		"ButtonTextAlign", ImGuiStyleVar_ButtonTextAlign,
		"ChildBorderSize", ImGuiStyleVar_ChildBorderSize,
		"ChildRounding", ImGuiStyleVar_ChildRounding,
		"FrameBorderSize", ImGuiStyleVar_FrameBorderSize,
		"FramePadding", ImGuiStyleVar_FramePadding,
		"FrameRounding", ImGuiStyleVar_FrameRounding,
		"GrabMinSize", ImGuiStyleVar_GrabMinSize,
		"GrabRounding", ImGuiStyleVar_GrabRounding,
		"IndentSpacing", ImGuiStyleVar_IndentSpacing,
		"ItemInnerSpacing", ImGuiStyleVar_ItemInnerSpacing,
		"ItemSpacing", ImGuiStyleVar_ItemSpacing,
		"PopupBorderSize", ImGuiStyleVar_PopupBorderSize,
		"PopupRounding", ImGuiStyleVar_PopupRounding,
		"ScrollbarRounding", ImGuiStyleVar_ScrollbarRounding,
		"ScrollbarSize", ImGuiStyleVar_ScrollbarSize,
		"SelectableTextAlign", ImGuiStyleVar_SelectableTextAlign,
		"TabRounding", ImGuiStyleVar_TabRounding,
		"WindowBorderSize", ImGuiStyleVar_WindowBorderSize,
		"WindowMinSize", ImGuiStyleVar_WindowMinSize,
		"WindowPadding", ImGuiStyleVar_WindowPadding,
		"WindowRounding", ImGuiStyleVar_WindowRounding,
		"WindowTitleAlign", ImGuiStyleVar_WindowTitleAlign
	);

	auto ig = lua["ig"] = lua.create_table_with();
	ig["preInit"] = [worker, device](SubRenderer* subrenderer) {
		// We'll be using multiple textures but only one per draw call, so we tell our subrenderer
		// about a dummy texture in preinit(),
		// and actually create the font texture and prepare for user textures in init()
		unsigned char pixels[4] = { 0 };
		new Texture(subrenderer, worker, pixels, 1, 1);

		// Not all of these are in use, but the pool is cheap and its what imgui
		// does in its vulkan+glfw example:
		// https://github.com/ocornut/imgui/blob/master/examples/example_glfw_vulkan/main.cpp
		VkDescriptorPoolSize pool_sizes[] =
		{
			{ VK_DESCRIPTOR_TYPE_SAMPLER, 1000 },
			{ VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, 1000 },
			{ VK_DESCRIPTOR_TYPE_SAMPLED_IMAGE, 1000 },
			{ VK_DESCRIPTOR_TYPE_STORAGE_IMAGE, 1000 },
			{ VK_DESCRIPTOR_TYPE_UNIFORM_TEXEL_BUFFER, 1000 },
			{ VK_DESCRIPTOR_TYPE_STORAGE_TEXEL_BUFFER, 1000 },
			{ VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER, 1000 },
			{ VK_DESCRIPTOR_TYPE_STORAGE_BUFFER, 1000 },
			{ VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER_DYNAMIC, 1000 },
			{ VK_DESCRIPTOR_TYPE_STORAGE_BUFFER_DYNAMIC, 1000 },
			{ VK_DESCRIPTOR_TYPE_INPUT_ATTACHMENT, 1000 }
		};
		VkDescriptorPoolCreateInfo pool_info = {};
		pool_info.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_POOL_CREATE_INFO;
		pool_info.flags = VK_DESCRIPTOR_POOL_CREATE_FREE_DESCRIPTOR_SET_BIT;
		pool_info.maxSets = 1000 * IM_ARRAYSIZE(pool_sizes);
		pool_info.poolSizeCount = (uint32_t)IM_ARRAYSIZE(pool_sizes);
		pool_info.pPoolSizes = pool_sizes;
		VK_CHECK_RESULT(vkCreateDescriptorPool(*device, &pool_info, nullptr, &subrenderer->imguiDescriptorPool));
	};
	ig["addFont"] = [worker](std::string fileTTF, float sizePixels) -> ImFont* {
		World* world = worker->getWorld();
		if (world->fonts == nullptr) {
			world->fonts = new ImFontAtlas();
			world->fonts->AddFontDefault();
		}
		return world->fonts->AddFontFromFileTTF(fileTTF.c_str(), sizePixels);
	};
	ig["init"] = [worker, device](SubRenderer* subrenderer) {
		// Create our font texture from ImGUI's pixel data
		ImGuiIO& io = ImGui::GetIO();
		unsigned char* pixels;
		int width, height;
		// We store a font atlas on the world so fonts can be added asynchronously,
		// so fonts can be loaded in a loading world - even if the active world is currently rendering
		World* world = worker->getWorld();
		if (world->fonts == nullptr) {
			world->fonts = new ImFontAtlas();
			world->fonts->AddFontDefault();
		}
		world->fonts->Build();
		world->fonts->GetTexDataAsRGBA32(&pixels, &width, &height);
		// TODO ensure only one ImGUI context per world?
		Texture* fontTexture = new Texture(subrenderer, worker, pixels, width, height);

		// Copied from renderer's addTexture function
		VkDescriptorSet descriptorSet;

		VkDescriptorSetAllocateInfo allocateInfo = {};
		allocateInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_ALLOCATE_INFO;
		allocateInfo.descriptorPool = subrenderer->imguiDescriptorPool;
		allocateInfo.descriptorSetCount = 1;
		allocateInfo.pSetLayouts = &subrenderer->descriptorSetLayout;

		VK_CHECK_RESULT(vkAllocateDescriptorSets(*device, &allocateInfo, &descriptorSet));

		VkDescriptorImageInfo descriptorImage[1] = { fontTexture->descriptor };
		VkWriteDescriptorSet writeDescriptor[1] = {};
		writeDescriptor[0].sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
		writeDescriptor[0].dstSet = descriptorSet;
		writeDescriptor[0].dstBinding = 1;
		writeDescriptor[0].descriptorCount = 1;
		writeDescriptor[0].descriptorType = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
		writeDescriptor[0].pImageInfo = descriptorImage;

		vkUpdateDescriptorSets(*device, 1, writeDescriptor, 0, nullptr);

		world->fonts->TexID = (ImTextureID)descriptorSet;
	};
	ig["newFrame"] = []() {
		ImGui_ImplGlfw_NewFrame();
		ImGui::NewFrame();
	};
	// TODO Is there a way to make this shorter/cleaner/somewhere els;
	ig["render"] = [engine, device](SecondaryCommandBuffer commandBuffer, SubRenderer* subrenderer) {
		ImGui::Render();

		auto draw_data = ImGui::GetDrawData();

		// Resize our vertex and index buffers if necessary
		size_t vertexSize = draw_data->TotalVtxCount;
		size_t indexSize = draw_data->TotalIdxCount;

		if (vertexSize == 0) return;

		if (engine->imguiVertexBufferSize < vertexSize) {
			size_t size = engine->imguiVertexBufferSize || 1;
			// Double size until its equal to or greater than minSize
			while (size < vertexSize)
				size <<= 1;
			// recreate our vertex buffer with the new size
			device->cleanupBuffer(engine->imguiVertexBuffer);
			engine->imguiVertexBuffer = device->createBuffer(
				size * sizeof(ImDrawVert),
				VK_BUFFER_USAGE_VERTEX_BUFFER_BIT,
				VMA_MEMORY_USAGE_CPU_TO_GPU);
			engine->imguiVertexBufferSize = size;
		}
		if (engine->imguiIndexBufferSize < indexSize) {
			size_t size = engine->imguiIndexBufferSize || 1;
			// Double size until its equal to or greater than minSize
			while (size < indexSize)
				size <<= 1;
			// recreate our index buffer with the new size
			device->cleanupBuffer(engine->imguiIndexBuffer);
			engine->imguiIndexBuffer = device->createBuffer(
				size * sizeof(ImDrawIdx),
				VK_BUFFER_USAGE_INDEX_BUFFER_BIT,
				VMA_MEMORY_USAGE_CPU_TO_GPU);
			engine->imguiIndexBufferSize = size;
		}

		// Fill our index and vertex buffers
		ImDrawVert* vtx_dst = (ImDrawVert*)engine->imguiVertexBuffer.map(vertexSize * sizeof(ImDrawVert), 0);
		ImDrawIdx* idx_dst = (ImDrawIdx*)engine->imguiIndexBuffer.map(indexSize * sizeof(ImDrawIdx), 0);
		for (int n = 0; n < draw_data->CmdListsCount; n++) {
			const ImDrawList* cmd_list = draw_data->CmdLists[n];
			engine->imguiVertexBuffer.copyTo(vtx_dst, cmd_list->VtxBuffer.Data, cmd_list->VtxBuffer.Size * sizeof(ImDrawVert));
			engine->imguiIndexBuffer.copyTo(idx_dst, cmd_list->IdxBuffer.Data, cmd_list->IdxBuffer.Size * sizeof(ImDrawIdx));
			vtx_dst += cmd_list->VtxBuffer.Size;
			idx_dst += cmd_list->IdxBuffer.Size;
		}
		// Flush our memory
		VmaAllocation flushAllocations[2] = { engine->imguiVertexBuffer.allocation, engine->imguiIndexBuffer.allocation };
		VkDeviceSize flushOffsets[2] = { 0, 0 };
		VkDeviceSize flushSizes[2] = { engine->imguiVertexBuffer.size, engine->imguiIndexBuffer.size };
		VK_CHECK_RESULT(vmaFlushAllocations(device->allocator, 2, flushAllocations, flushOffsets, flushSizes));
		// Unmap our memory
		engine->imguiVertexBuffer.unmap();
		engine->imguiIndexBuffer.unmap();

		// Submit our push constants
		float scale[2];
		scale[0] = 2.0f / draw_data->DisplaySize.x;
		scale[1] = 2.0f / draw_data->DisplaySize.y;
		float translate[2];
		translate[0] = -1.0f - draw_data->DisplayPos.x * scale[0];
		translate[1] = -1.0f - draw_data->DisplayPos.y * scale[1];

		vkCmdPushConstants(commandBuffer, subrenderer->pipelineLayout, VK_SHADER_STAGE_VERTEX_BIT, sizeof(float) * 0, sizeof(float) * 2, scale);
		vkCmdPushConstants(commandBuffer, subrenderer->pipelineLayout, VK_SHADER_STAGE_VERTEX_BIT, sizeof(float) * 2, sizeof(float) * 2, translate);

		// Bind our vertex and index buffers
		VkBuffer vertexBuffers[] = { engine->imguiVertexBuffer };
		VkDeviceSize offsets[] = { 0 };
		vkCmdBindVertexBuffers(commandBuffer, 0, 1, vertexBuffers, offsets);
		vkCmdBindIndexBuffer(commandBuffer, engine->imguiIndexBuffer, 0, sizeof(ImDrawIdx) == 2 ? VK_INDEX_TYPE_UINT16 : VK_INDEX_TYPE_UINT32);

		// Will project scissor/clipping rectangles into framebuffer space
		ImVec2 clip_off = draw_data->DisplayPos;         // (0,0) unless using multi-viewports
		ImVec2 clip_scale = draw_data->FramebufferScale; // (1,1) unless using retina display which are often (2,2)

		// Render command lists
		// (Because we merged all buffers into a single one, we maintain our own offset into them)
		int global_vtx_offset = 0;
		int global_idx_offset = 0;
		for (int n = 0; n < draw_data->CmdListsCount; n++) {
			const ImDrawList* cmd_list = draw_data->CmdLists[n];
			for (int cmd_i = 0; cmd_i < cmd_list->CmdBuffer.Size; cmd_i++) {
				const ImDrawCmd* pcmd = &cmd_list->CmdBuffer[cmd_i];
				if (pcmd->UserCallback != NULL) {
					// User callback, registered via ImDrawList::AddCallback()
					// (ImDrawCallback_ResetRenderState is a special callback value used by the user to request the renderer to reset render state.)
					if (pcmd->UserCallback == ImDrawCallback_ResetRenderState) {
						// TODO how to implement this?
					}
					else pcmd->UserCallback(cmd_list, pcmd);
				} else {
					// Project scissor/clipping rectangles into framebuffer space
					ImVec4 clip_rect;
					clip_rect.x = (pcmd->ClipRect.x - clip_off.x) * clip_scale.x;
					clip_rect.y = (pcmd->ClipRect.y - clip_off.y) * clip_scale.y;
					clip_rect.z = (pcmd->ClipRect.z - clip_off.x) * clip_scale.x;
					clip_rect.w = (pcmd->ClipRect.w - clip_off.y) * clip_scale.y;

					if (clip_rect.x < engine->renderer.swapChainExtent.width && clip_rect.y < engine->renderer.swapChainExtent.height &&
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
						vkCmdSetScissor(commandBuffer, 0, 1, &scissor);

						// Bind descriptor set with font or user texture
						// TODO slight inefficiency when SubRenderer binds the font descriptor set
						VkDescriptorSet descSet[1] = { (VkDescriptorSet)pcmd->TextureId };
						vkCmdBindDescriptorSets(commandBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS, subrenderer->pipelineLayout, 0, 1, descSet, 0, NULL);

						// Draw
						vkCmdDrawIndexed(commandBuffer, pcmd->ElemCount, 1, pcmd->IdxOffset + global_idx_offset, pcmd->VtxOffset + global_vtx_offset, 0);
					}
				}
			}
			global_idx_offset += cmd_list->IdxBuffer.Size;
			global_vtx_offset += cmd_list->VtxBuffer.Size;
		}
	};
	ig["setNextWindowPos"] = [](int x, int y) { SetNextWindowPos(ImVec2(x, y)); };
	ig["setNextWindowSize"] = [](int width, int height) { SetNextWindowSize(ImVec2(width, height)); };
	ig["setNextWindowFocus"] = &SetNextWindowFocus;
	ig["setWindowFontScale"] = &SetWindowFontScale;
	ig["lock"] = []() { imguiMutex.lock(); };
	ig["release"] = []() { imguiMutex.unlock(); };
	ig["beginWindow"] = [](std::string title, bool* p_open, std::vector<ImGuiWindowFlags> flags) {
		ImGuiWindowFlags windowFlags = 0;
		for (auto flag : flags)
			windowFlags |= flag;
		Begin(title.c_str(), p_open, windowFlags);
	};
	ig["endWindow"] = &End;
	ig["openPopup"] = &OpenPopup;
	ig["beginPopupModal"] = [](std::string title, bool* p_open, std::vector<ImGuiWindowFlags> flags) {
		ImGuiWindowFlags windowFlags = 0;
		for (auto flag : flags)
			windowFlags |= flag;
		BeginPopupModal(title.c_str(), p_open, windowFlags);
	};
	ig["endPopup"] = &EndPopup;
	ig["beginChild"] = [](std::string title, glm::vec2 size, bool border, std::vector<ImGuiWindowFlags> flags) {
		ImGuiWindowFlags windowFlags = 0;
		for (auto flag : flags)
			windowFlags |= flag;
		BeginChild(title.c_str(), ImVec2(size.x, size.y), border, windowFlags);
	};
	ig["endChild"] = &EndChild;
	ig["beginTooltip"] = []() { BeginTooltip(); };
	ig["endTooltip"] = []() { EndTooltip(); };
	ig["logToClipboard"] = []() { LogToClipboard(); };
	ig["logFinish"] = &LogFinish;
	ig["pushStyleColor"] = [](ImGuiCol idx, glm::vec4 color) { PushStyleColor(idx, ImVec4(color.r, color.g, color.b, color.a)); };
	ig["popStyleColor"] = [](int amount) { PopStyleColor(amount); };
	ig["pushStyleVar"] = sol::overload([](ImGuiStyleVar idx, float val) { PushStyleVar(idx, val); }, [](ImGuiStyleVar idx, float v1, float v2) { PushStyleVar(idx, ImVec2(v1, v2)); });
	ig["popStyleVar"] = [](int amount) { PopStyleVar(amount); };
	ig["pushTextWrapPos"] = [](float pos) { PushTextWrapPos(pos); };
	ig["popTextWrapPos"] = []() { PopTextWrapPos(); };
	ig["pushFont"] = [](ImFont* font) { PushFont(font); };
	ig["popFont"] = []() { PopFont(); };
	ig["pushItemWidth"] = [](float width) { PushItemWidth(width); };
	ig["popItemWidth"] = []() { PopItemWidth(); };
	ig["setKeyboardFocusHere"] = sol::overload([]() { SetKeyboardFocusHere(); }, & SetKeyboardFocusHere);
	ig["getScrollY"] = &GetScrollX;
	ig["getScrollY"] = &GetScrollY;
	ig["setScrollHereX"] = sol::overload([]() { SetScrollHereX(); }, &SetScrollHereX);
	ig["setScrollHereY"] = sol::overload([]() { SetScrollHereY(); }, &SetScrollHereY);
	ig["getScrollMaxX"] = &GetScrollMaxX;
	ig["getScrollMaxY"] = &GetScrollMaxY;
	ig["calcTextSize"] = [](std::string text) -> std::tuple<float, float> {
		ImVec2 size = CalcTextSize(text.c_str());
		return std::make_tuple(size.x, size.y);
	};
	ig["image"] = [](Texture* texture, glm::vec2 size, glm::vec4 uv) {
		// Note we flip q and t - that's because vulkan flips the y component compared to opengl
		Image((ImTextureID)texture->imguiTexId, ImVec2(size.x, size.y), ImVec2(uv.p, uv.t), ImVec2(uv.s, uv.q));
	};
	ig["sameLine"] = []() { SameLine(); };
	ig["getCursorPos"] = []() -> glm::vec2 {
		auto pos = GetCursorPos();
		return glm::vec2(pos.x, pos.y);
	},
	ig["setCursorPos"] = [](float x, float y) { SetCursorPos(ImVec2(x, y)); };
	ig["text"] = [](std::string text) { Text(text.c_str()); };
	ig["textWrapped"] = [](std::string text) { TextWrapped(text.c_str()); };
	ig["textUnformatted"] = [](std::string text) { TextUnformatted(text.c_str()); };
	ig["button"] = sol::overload([](std::string text) -> bool { return Button(text.c_str()); }, [](std::string text, glm::vec2 size) -> bool { return Button(text.c_str(), ImVec2(size.x, size.y)); });
	ig["checkbox"] = [](std::string label, bool value) -> bool {
		Checkbox(label.c_str(), &value);
		return value;
	};
	ig["progressBar"] = [](float percent) { ProgressBar(percent); };
	ig["isItemHovered"] = []() -> bool { return IsItemHovered(); };
	ig["isMouseClicked"] = []() -> bool { return IsMouseClicked(ImGuiMouseButton_Left); };
	ig["separator"] = &Separator;
	ig["spacing"] = &Spacing;
	ig["showDemoWindow"] = []() { ShowDemoWindow(); };
	ig["inputText"] = [](std::string label, std::string input, std::vector<ImGuiInputTextFlags> flags, LuaVal* self, sol::function callback) -> std::tuple<bool, std::string> {
		ImGuiInputTextFlags inputTextFlags = 0;
		for (auto flag : flags)
			inputTextFlags |= flag;
		bool result = InputText(label.c_str(), &input, inputTextFlags, InputTextCallback, new LuaCallback(self, callback));
		// Since we can't create a string pointer in our lua code, we return the updated string alongside the input text's return value
		return std::make_tuple(result, input);
	};
	lua.new_usertype<ImGuiTextFilter>("textFilter",
		sol::constructors<ImGuiTextFilter()>(),
		"draw", [](ImGuiTextFilter* filter, std::string label) { filter->Draw(label.c_str()); },
		"check", [](ImGuiTextFilter* filter, std::string text) -> bool { return filter->PassFilter(text.c_str()); }
	);
	lua.new_usertype<ImGuiTextEditCallbackData>("textEditCallbackData",
		"new", sol::no_constructor,
		"eventFlag", &ImGuiTextEditCallbackData::EventFlag,
		"getKey", [](ImGuiTextEditCallbackData* data) -> int {
			// We map imgui keys to GLFW keys so we only need one keys enum
			switch (data->EventKey) {
			case ImGuiKey_Tab:
				return GLFW_KEY_TAB;
			case ImGuiKey_LeftArrow:
				return GLFW_KEY_LEFT;
			case ImGuiKey_RightArrow:
				return GLFW_KEY_RIGHT;
			case ImGuiKey_UpArrow:
				return GLFW_KEY_UP;
			case ImGuiKey_DownArrow:
				return GLFW_KEY_DOWN;
			case ImGuiKey_PageDown:
				return GLFW_KEY_PAGE_DOWN;
			case ImGuiKey_PageUp:
				return GLFW_KEY_PAGE_UP;
			case ImGuiKey_Home:
				return GLFW_KEY_HOME;
			case ImGuiKey_End:
				return GLFW_KEY_END;
			case ImGuiKey_Insert:
				return GLFW_KEY_INSERT;
			case ImGuiKey_Delete:
				return GLFW_KEY_DELETE;
			case ImGuiKey_Backspace:
				return GLFW_KEY_BACKSPACE;
			case ImGuiKey_Space:
				return GLFW_KEY_SPACE;
			case ImGuiKey_Enter:
				return GLFW_KEY_ENTER;
			case ImGuiKey_Escape:
				return GLFW_KEY_ESCAPE;
			case ImGuiKey_A:
				return GLFW_KEY_A;
			case ImGuiKey_C:
				return GLFW_KEY_C;
			case ImGuiKey_V:
				return GLFW_KEY_V;
			case ImGuiKey_X:
				return GLFW_KEY_X;
			case ImGuiKey_Y:
				return GLFW_KEY_Y;
			case ImGuiKey_Z:
				return GLFW_KEY_Z;
			default:
				return GLFW_KEY_UNKNOWN;
			}
		},
		"getText", [](ImGuiTextEditCallbackData* data) -> std::string { return std::string(data->Buf, data->BufTextLen); },
		"cursorPos", & ImGuiTextEditCallbackData::CursorPos
	);
	lua.new_usertype<ImFont>("font", sol::no_constructor);
}
