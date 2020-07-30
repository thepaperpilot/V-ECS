#include "SubRenderer.h"

#include "Renderer.h"
#include "Model.h"
#include "SecondaryCommandBuffer.h"
#include "Texture.h"
#include "VertexLayout.h"
#include "../ecs/World.h"
#include "../ecs/WorldLoadStatus.h"
#include "../engine/Device.h"
#include "../jobs/Worker.h"
#include "../lua/LuaVal.h"
#include "../util/VulkanUtils.h"

#include <vulkan/vulkan.h>

using namespace vecs;

SubRenderer::SubRenderer(LuaVal* config, Worker* worker, Device* device, Renderer* renderer, DependencyNodeLoadStatus* status)
    : config(config), worker(worker), device(device), renderer(renderer) {
    vertexLayout = new VertexLayout(config->get("vertexLayout"));

    LuaVal preInit = config->get("preInit");
    if (preInit.type == LUA_TYPE_FUNCTION) {
        sol::load_result loadResult = worker->lua.load(std::get<sol::bytecode>(preInit.value).as_string_view());
        if (!loadResult.valid()) {
            sol::error err = loadResult;
            Debugger::addLog(DEBUG_LEVEL_ERROR, "[LUA] " + std::string(err.what()));
            status->preInitStatus = DEPENDENCY_FUNCTION_ERROR;
            // TODO cancel world loading and future jobs
            return;
        }

        auto result = loadResult(config, this);
        if (!result.valid()) {
            sol::error err = result;
            Debugger::addLog(DEBUG_LEVEL_ERROR, "[LUA] " + std::string(err.what()));
            status->preInitStatus = DEPENDENCY_FUNCTION_ERROR;
            // TODO cancel world loading and future jobs
            return;
        }
    }

    numTextures = static_cast<uint32_t>(textures.size());
    imageInfos.reserve(numTextures);
    for (auto texture : textures)
        imageInfos.emplace_back(texture->descriptor);

    for (auto model : models) {
        numTextures += static_cast<uint32_t>(model->textures.size());
        for (auto texture : model->textures)
            imageInfos.emplace_back(texture.descriptor);
        if (model->hasMaterial) {
            switch (model->materialShaderStage) {
            case VK_SHADER_STAGE_VERTEX_BIT:
                numVertexUniforms++;
                vertexUniformBufferInfos.emplace_back(model->materialBufferInfo);
                break;
            case VK_SHADER_STAGE_FRAGMENT_BIT:
                numFragmentUniforms++;
                fragmentUniformBufferInfos.emplace_back(model->materialBufferInfo);
                break;
            }
        }
    }

    createDescriptorLayoutBindings();
    createDescriptorSetLayout();

    createShaderStages();
    createVertexInput();
    createInputAssembly();
    createRasterizer();
    createMultisampling();
    createDepthStencil();
    createColorBlendAttachments();
    createDynamicStates();
    createPushConstantRanges();

    createGraphicsPipeline();

    // Cleanup our shader modules
    cleanShaderModules();
}

SecondaryCommandBuffer* SubRenderer::startRendering(Worker* worker) {
    // Find index of the descriptor pool to use
    uint32_t imageIndex = static_cast<uint32_t>(allocatedDescriptorSets);
    allocatedDescriptorSets++;

    // Resize array of descriptor pools as necessary
    if (imageIndex >= numDescriptorPools) {
        // We need to lock the descriptor pools vector so we're not accessing elements during re-allocation.
        // We lock it before calculating the new size so that if two threads need to add elements at the same
        // time they'll have the most up-to-date size of the vector.
        descriptorPoolsMutex.lock();
        uint32_t newSize = numDescriptorPools;
        uint32_t oldSize = newSize;
        if (newSize == 0) newSize = 1;
        while (imageIndex >= newSize)
            newSize *= 2;

        // Check if they're different, which could be the case if another thread already added new descriptor pools
        if (newSize != oldSize) {
            // Create new descriptor pools, each for renderer->imageCount descriptor sets. Since each descriptor pool can only be used
            // on a single thread, we can't destroy and re-create one pool whenever we need more descritptor sets, because that'd interfere
            // with the other threads already using descriptorSets from the pool we're destroying. Having this array of descriptor pools and associated
            // descriptor sets allows us to have one pool on each thread, resetting each frame. If we need more pools in a single frame, we can add them to the
            // list without needing to destroy any already created pools or sets.
            descriptorPools.reserve(newSize);
            descriptorSets.reserve(newSize);
            numDescriptorPools = newSize;
            for (uint32_t i = oldSize; i < newSize; i++) {
                VkDescriptorPool pool = createDescriptorPool(renderer->imageCount);
                descriptorPools.emplace_back(pool);
                descriptorSets.emplace_back(createDescriptorSets(pool, renderer->imageCount));
            }
        }
        descriptorPoolsMutex.unlock();
    }

    // Get an active command buffer
    SecondaryCommandBuffer* activeCommandBuffer = new SecondaryCommandBuffer(renderer, worker->getCommandBuffer(), renderer->imageIndex);

    // Set viewport and scissor
    VkViewport viewport = {};
    viewport.x = 0.0f;
    viewport.y = 0.0f;
    viewport.width = (float)renderer->swapChainExtent.width;
    viewport.height = (float)renderer->swapChainExtent.height;
    viewport.minDepth = 0.0f;
    viewport.maxDepth = 1.0f;
    vkCmdSetViewport(*activeCommandBuffer, 0, 1, &viewport);

    VkRect2D scissor = {};
    scissor.offset = { 0,0 };
    scissor.extent = renderer->swapChainExtent;
    vkCmdSetScissor(*activeCommandBuffer, 0, 1, &scissor);

    // Bind our descriptor sets
    if (bindings.size() > 0) {
        descriptorPoolsMutex.lock_shared();
        vkCmdBindDescriptorSets(*activeCommandBuffer,
            VK_PIPELINE_BIND_POINT_GRAPHICS,
            pipelineLayout, 0,
            1, &descriptorSets[imageIndex][renderer->imageIndex],
            0, NULL);
        descriptorPoolsMutex.unlock_shared();
    }

    // Bind our graphics pipeline
    vkCmdBindPipeline(*activeCommandBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS, graphicsPipeline);

    return activeCommandBuffer;
}

void SubRenderer::finishRendering(VkCommandBuffer activeCommandBuffer) {
    // End the command buffer
    vkEndCommandBuffer(activeCommandBuffer);
    renderer->secondaryBufferMutex.lock();
    renderer->secondaryBuffers.push_back(activeCommandBuffer);
    renderer->secondaryBufferMutex.unlock();
}

void SubRenderer::windowRefresh(int imageCount) {
    // No need to worry about multithreading issues because this will only be called during glfw's pollEvents call,
    // during which there *should be* no rendering jobs
    if (numDescriptorPools <= 0 || descriptorSets[0].size() == imageCount) return;

    for (int i = 0; i < descriptorPools.size(); i++) {
        vkDestroyDescriptorPool(*device, descriptorPools[i], nullptr);
        descriptorPools[i] = createDescriptorPool(imageCount);
        descriptorSets[i] = createDescriptorSets(descriptorPools[i], imageCount);
    }
}

void SubRenderer::cleanup() {
    // Destroy any textures and models
    for (auto texture : textures)
        texture->cleanup();
    for (auto model : models)
        model->cleanup();

    // Destroy our descriptor set layout
    vkDestroyDescriptorSetLayout(*device, descriptorSetLayout, nullptr);
    for (auto descriptorPool : descriptorPools)
        vkDestroyDescriptorPool(*device, descriptorPool, nullptr);

    // Destroy our imgui descriptor pool
    vkDestroyDescriptorPool(*device, imguiDescriptorPool, nullptr);

    // Destroy our graphics pipeline
    vkDestroyPipeline(*device, graphicsPipeline, nullptr);
    vkDestroyPipelineLayout(*device, pipelineLayout, nullptr);
}

void SubRenderer::addVertexUniform(Buffer* buffer, VkDeviceSize size) {    
    VkDescriptorBufferInfo bufferInfo = {};
    bufferInfo.buffer = *buffer;
    bufferInfo.offset = 0;
    bufferInfo.range = size;

    numVertexUniforms++;
    vertexUniformBufferInfos.emplace_back(bufferInfo);
}

void SubRenderer::addFragmentUniform(Buffer* buffer, VkDeviceSize size) {
    VkDescriptorBufferInfo bufferInfo = {};
    bufferInfo.buffer = *buffer;
    bufferInfo.offset = 0;
    bufferInfo.range = size;

    numFragmentUniforms++;
    fragmentUniformBufferInfos.emplace_back(bufferInfo);
}

// TODO refactor vertex/fragment uniform buffer-specific code to utilize a vertex stage -> uniform buffers map
void SubRenderer::createDescriptorLayoutBindings() {
    if (numTextures > 0) {
        VkDescriptorSetLayoutBinding samplerLayoutBinding = {};
        samplerLayoutBinding.binding = 1;
        samplerLayoutBinding.descriptorCount = numTextures;
        samplerLayoutBinding.descriptorType = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
        samplerLayoutBinding.pImmutableSamplers = immutableSamplers.data();
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
}

void SubRenderer::createDescriptorSetLayout() {
    VkDescriptorSetLayoutCreateInfo layoutInfo = {};
    layoutInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_CREATE_INFO;
    layoutInfo.bindingCount = static_cast<uint32_t>(bindings.size());
    layoutInfo.pBindings = bindings.data();

    VK_CHECK_RESULT(vkCreateDescriptorSetLayout(*device, &layoutInfo, nullptr, &descriptorSetLayout));
}

VkDescriptorPool SubRenderer::createDescriptorPool(size_t imageCount) {
    VkDescriptorPool descriptorPool;
    if (bindings.size() == 0) return nullptr;

    std::vector<VkDescriptorPoolSize> poolSizes(bindings.size());
    for (int i = (int)bindings.size() - 1; i >= 0; i--) {
        poolSizes[i].type = bindings[i].descriptorType;
        poolSizes[i].descriptorCount = static_cast<uint32_t>(imageCount)* bindings[i].descriptorCount;
    }

    VkDescriptorPoolCreateInfo poolInfo = {};
    poolInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_POOL_CREATE_INFO;
    poolInfo.poolSizeCount = static_cast<uint32_t>(poolSizes.size());
    poolInfo.pPoolSizes = poolSizes.data();
    poolInfo.maxSets = static_cast<uint32_t>(imageCount);

    VK_CHECK_RESULT(vkCreateDescriptorPool(*device, &poolInfo, nullptr, &descriptorPool));
    return descriptorPool;
}

std::vector<VkDescriptorSet> SubRenderer::createDescriptorSets(VkDescriptorPool descriptorPool, size_t imageCount) {
    std::vector<VkDescriptorSet> descriptorSets;
    descriptorSets.resize(imageCount);
    if (bindings.size() == 0) return descriptorSets;

    std::vector<VkDescriptorSetLayout> layouts(imageCount, descriptorSetLayout);
    VkDescriptorSetAllocateInfo allocInfo = {};
    allocInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_ALLOCATE_INFO;
    allocInfo.descriptorPool = descriptorPool;
    allocInfo.descriptorSetCount = static_cast<uint32_t>(imageCount);
    allocInfo.pSetLayouts = layouts.data();

    VK_CHECK_RESULT(vkAllocateDescriptorSets(*device, &allocInfo, descriptorSets.data()));

    for (size_t i = 0; i < imageCount; i++) {
        std::vector<VkWriteDescriptorSet> descriptorWrites = getDescriptorWrites(descriptorSets[i]);

        vkUpdateDescriptorSets(*device, static_cast<uint32_t>(descriptorWrites.size()), descriptorWrites.data(), 0, nullptr);
    }

    return descriptorSets;
}

std::vector<VkWriteDescriptorSet> SubRenderer::getDescriptorWrites(VkDescriptorSet descriptorSet) {
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

void SubRenderer::createShaderStages() {
    LuaVal shaders = config->get("shaders");

    if (shaders.type != LUA_TYPE_TABLE) return;

    for (auto kvp : *std::get<LuaVal::MapType*>(shaders.value)) {
        if (kvp.first.type != LUA_TYPE_STRING) {
            Debugger::addLog(DEBUG_LEVEL_WARN, "Attempted to load shader with non-string filename key");
            continue;
        }
        if (kvp.second.type != LUA_TYPE_NUMBER) {
            Debugger::addLog(DEBUG_LEVEL_WARN, "Attempted to load shader with non-numeric value");
            continue;
        }
        std::string name = std::get<std::string>(kvp.first.value);
        VkShaderStageFlagBits stage = kvp.second.getEnum<VkShaderStageFlagBits>();
        VkShaderModule shaderModule = getCompiledShader(&device->logical, name);
        shaderModules.emplace_back(shaderModule);

        VkPipelineShaderStageCreateInfo shaderStageInfo = {};
        shaderStageInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO;
        shaderStageInfo.stage = stage;
        shaderStageInfo.module = shaderModule;
        shaderStageInfo.pName = "main";
        shaderStages.emplace_back(shaderStageInfo);
    }
}

void SubRenderer::createVertexInput() {
    // Describe the format of the vertex data being passed to the vertex shader
    vertexInput.sType = VK_STRUCTURE_TYPE_PIPELINE_VERTEX_INPUT_STATE_CREATE_INFO;
    vertexInput.pNext = nullptr;
    vertexInput.flags = 0;
    vertexInput.vertexBindingDescriptionCount = 1;
    vertexInput.vertexAttributeDescriptionCount = static_cast<uint32_t>(vertexLayout->attributeDescriptions.size());
    vertexInput.pVertexBindingDescriptions = &vertexLayout->bindingDescription;
    vertexInput.pVertexAttributeDescriptions = vertexLayout->attributeDescriptions.data();
}

void SubRenderer::createInputAssembly() {
    // Describe what kind of geometry will be drawn from the vertices
    inputAssembly.sType = VK_STRUCTURE_TYPE_PIPELINE_INPUT_ASSEMBLY_STATE_CREATE_INFO;
    inputAssembly.pNext = nullptr;
    inputAssembly.flags = 0;
    inputAssembly.topology = VK_PRIMITIVE_TOPOLOGY_TRIANGLE_LIST;
    inputAssembly.primitiveRestartEnable = VK_FALSE;
}

void SubRenderer::createRasterizer() {
    // using config.get_or was giving me compiler errors so I'm using this method instead
    LuaVal cullMode = config->get("cullMode");

    // Describe our rasterizer
    rasterizer.sType = VK_STRUCTURE_TYPE_PIPELINE_RASTERIZATION_STATE_CREATE_INFO;
    rasterizer.pNext = nullptr;
    rasterizer.flags = 0;
    // This option would tell it to clamp objects that are too near or far instead of discarding them
    // requires turning on a GPU-specific feature
    rasterizer.depthClampEnable = VK_FALSE;
    // This option would prevent the rasterizer from passing any data to the frame buffer
    rasterizer.rasterizerDiscardEnable = VK_FALSE;
    // This option determines if we should fill in each triangle or just keep the wireframe or vertices
    // LINE and POINT modes require enabling a GPU-specific feature
    rasterizer.polygonMode = VK_POLYGON_MODE_FILL;
    // Line width higher than 1 requires the wideLines GPU feature
    rasterizer.lineWidth = 1.0f;
    // This is a setting that can help with shadow mapping
    rasterizer.depthBiasEnable = VK_FALSE;
    rasterizer.depthBiasClamp = 0;
    // These two options determine how to calculate culling
    rasterizer.cullMode = cullMode.type == LUA_TYPE_NUMBER ? cullMode.getEnum<VkCullModeFlagBits>() : VK_CULL_MODE_BACK_BIT;
    rasterizer.frontFace = VK_FRONT_FACE_COUNTER_CLOCKWISE;
}

void SubRenderer::createMultisampling() {
    // Describe our multisampling AA
    multisampling.sType = VK_STRUCTURE_TYPE_PIPELINE_MULTISAMPLE_STATE_CREATE_INFO;
    multisampling.pNext = nullptr;
    multisampling.flags = 0;
    multisampling.sampleShadingEnable = VK_FALSE;
    multisampling.alphaToOneEnable = VK_FALSE;
    multisampling.alphaToCoverageEnable = VK_FALSE;
    multisampling.pSampleMask = nullptr;
    multisampling.rasterizationSamples = VK_SAMPLE_COUNT_1_BIT;
}

void SubRenderer::createDepthStencil() {
    LuaVal depthTest = config->get("performDepthTest");
    VkBool32 performDepthTest = depthTest.type == LUA_TYPE_BOOL ? std::get<bool>(depthTest.value) : VK_TRUE;

    // Describe the depth and stencil buffer
    depthStencil.sType = VK_STRUCTURE_TYPE_PIPELINE_DEPTH_STENCIL_STATE_CREATE_INFO;
    depthStencil.pNext = nullptr;
    depthStencil.flags = 0;
    depthStencil.depthTestEnable = performDepthTest;
    depthStencil.depthWriteEnable = performDepthTest;
    depthStencil.depthCompareOp = VK_COMPARE_OP_LESS;
    depthStencil.depthBoundsTestEnable = VK_FALSE;
    depthStencil.minDepthBounds = 0;
    depthStencil.maxDepthBounds = 1;
    depthStencil.stencilTestEnable = VK_FALSE;
    depthStencil.front = {};
    depthStencil.back = {};
}

void SubRenderer::createColorBlendAttachments() {
    // First part is for our frame buffers, of which we only have one
    colorBlendAttachment.colorWriteMask = VK_COLOR_COMPONENT_R_BIT | VK_COLOR_COMPONENT_G_BIT | VK_COLOR_COMPONENT_B_BIT | VK_COLOR_COMPONENT_A_BIT;
    // We'll make it blend colors based on the alpha bit
    colorBlendAttachment.blendEnable = VK_TRUE;
    colorBlendAttachment.srcColorBlendFactor = VK_BLEND_FACTOR_SRC_ALPHA;
    colorBlendAttachment.dstColorBlendFactor = VK_BLEND_FACTOR_ONE_MINUS_SRC_ALPHA;
    colorBlendAttachment.colorBlendOp = VK_BLEND_OP_ADD;
    colorBlendAttachment.srcAlphaBlendFactor = VK_BLEND_FACTOR_ONE;
    colorBlendAttachment.dstAlphaBlendFactor = VK_BLEND_FACTOR_ZERO;
    colorBlendAttachment.alphaBlendOp = VK_BLEND_OP_ADD;

    // Second part of color blending are global settings
    colorBlending.sType = VK_STRUCTURE_TYPE_PIPELINE_COLOR_BLEND_STATE_CREATE_INFO;
    colorBlending.pNext = nullptr;
    colorBlending.flags = 0;
    // Setting this to true will ignore all the frame buffer-specific color blending
    colorBlending.logicOpEnable = VK_FALSE;
    colorBlending.attachmentCount = 1;
    colorBlending.pAttachments = &colorBlendAttachment;
    colorBlending.blendConstants[0] = 0;
    colorBlending.blendConstants[1] = 0;
    colorBlending.blendConstants[2] = 0;
    colorBlending.blendConstants[3] = 0;
}

void SubRenderer::createDynamicStates() {
    // Specify what properties we want to specify at draw-time
    dynamicState.sType = VK_STRUCTURE_TYPE_PIPELINE_DYNAMIC_STATE_CREATE_INFO;
    dynamicState.pNext = nullptr;
    dynamicState.flags = 0;
    dynamicState.dynamicStateCount = static_cast<uint32_t>(dynamicStates.size());
    dynamicState.pDynamicStates = dynamicStates.data();
}

void SubRenderer::createPushConstantRanges() {
    LuaVal sizeVal = config->get("pushConstantsSize");
    uint32_t size = sizeVal.type == LUA_TYPE_NUMBER ? (uint32_t)std::get<double>(sizeVal.value) : 0;

    VkPushConstantRange pushConstantRange = {};
    pushConstantRange.stageFlags = VK_SHADER_STAGE_VERTEX_BIT;
    pushConstantRange.offset = 0;
    pushConstantRange.size = size;

    pushConstantRanges = { pushConstantRange };
}

void SubRenderer::createGraphicsPipeline() {
    // Describe the region of the framebuffer we want to draw to with placeholder values
    // We'll be setting these dynamically when building our command buffers
    VkViewport viewport = {};
    viewport.x = 0.0f;
    viewport.y = 0.0f;
    viewport.width = 1;
    viewport.height = 1;
    viewport.minDepth = 0.0f;
    viewport.maxDepth = 1.0f;

    // Describe the size of the space to be used to store the pixels to draw
    // We want it to be the same size as the viewport/swap chain extent
    VkRect2D scissor = {};
    scissor.offset = { 0, 0 };
    scissor.extent = { 1, 1 };

    // Combine the viewport and scissor into one viewportState
    VkPipelineViewportStateCreateInfo viewportState = {};
    viewportState.sType = VK_STRUCTURE_TYPE_PIPELINE_VIEWPORT_STATE_CREATE_INFO;
    viewportState.viewportCount = 1;
    viewportState.pViewports = &viewport;
    viewportState.scissorCount = 1;
    viewportState.pScissors = &scissor;

    // Save our pipeline layout
    // We add our descriptor set and push constant ranges here
    VkPipelineLayoutCreateInfo pipelineLayoutInfo = {};
    pipelineLayoutInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_LAYOUT_CREATE_INFO;
    pipelineLayoutInfo.setLayoutCount = 1;
    pipelineLayoutInfo.pSetLayouts = &descriptorSetLayout;
    pipelineLayoutInfo.pushConstantRangeCount = static_cast<uint32_t>(pushConstantRanges.size());
    pipelineLayoutInfo.pPushConstantRanges = pushConstantRanges.data();

    VK_CHECK_RESULT(vkCreatePipelineLayout(*device, &pipelineLayoutInfo, nullptr, &pipelineLayout));

    // Bring everything together for creating the actual pipeline
    VkGraphicsPipelineCreateInfo pipelineInfo = {};
    pipelineInfo.sType = VK_STRUCTURE_TYPE_GRAPHICS_PIPELINE_CREATE_INFO;
    pipelineInfo.stageCount = static_cast<uint32_t>(shaderStages.size());
    pipelineInfo.pStages = shaderStages.data();
    pipelineInfo.pVertexInputState = &vertexInput;
    pipelineInfo.pInputAssemblyState = &inputAssembly;
    pipelineInfo.pViewportState = &viewportState;
    pipelineInfo.pRasterizationState = &rasterizer;
    pipelineInfo.pMultisampleState = &multisampling;
    pipelineInfo.pDepthStencilState = &depthStencil;
    pipelineInfo.pColorBlendState = &colorBlending;
    pipelineInfo.pDynamicState = &dynamicState;
    pipelineInfo.layout = pipelineLayout;
    pipelineInfo.renderPass = renderer->renderPass;
    pipelineInfo.subpass = 0;

    // Create the graphics pipeline!
    VK_CHECK_RESULT(vkCreateGraphicsPipelines(*device, VK_NULL_HANDLE, 1, &pipelineInfo, nullptr, &graphicsPipeline));
}

void SubRenderer::cleanShaderModules() {
    for (auto shader : shaderModules) {
        vkDestroyShaderModule(*device, shader, nullptr);
    }
}
