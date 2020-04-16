#include "SubRenderer.h"
#include "Renderer.h"
#include "../engine/Device.h"
#include "../util/VulkanUtils.h"

#include <vulkan/vulkan.h>

using namespace vecs;

SubRenderer::SubRenderer(Device* device, Renderer* renderer, sol::table worldConfig, sol::table config) {
    this->device = device;
    this->renderer = renderer;
    this->config = config;

    vertexLayout = new VertexLayout(config.get_or("vertexLayout", sol::table()));

    if (config["preInit"].get_type() == sol::type::function) {
        auto result = config["preInit"](config, worldConfig, this);
        if (!result.valid()) {
            sol::error err = result;
            Debugger::addLog(DEBUG_LEVEL_ERROR, "[LUA] " + std::string(err.what()));
        }
    }

    numTextures = textures.size();
    imageInfos.reserve(numTextures);
    for (auto texture : textures)
        imageInfos.emplace_back(texture->descriptor);

    for (auto model : models) {
        numTextures += model->textures.size();
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
    createDescriptorPool(renderer->imageCount);
    createDescriptorSets(renderer->imageCount);

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
    createInheritanceInfo(renderer->imageCount);

    // Cleanup our shader modules
    cleanShaderModules();

    commandBuffers = device->createCommandBuffers(VK_COMMAND_BUFFER_LEVEL_SECONDARY, renderer->imageCount);
}

void SubRenderer::buildCommandBuffer(sol::table worldConfig) {
    uint32_t imageIndex = renderer->imageIndex;

    // Begin the command buffer
    activeCommandBuffer = commandBuffers[imageIndex];
    device->beginSecondaryCommandBuffer(activeCommandBuffer, &inheritanceInfo[imageIndex]);

    // Set viewport and scissor
    VkViewport viewport = {};
    viewport.x = 0.0f;
    viewport.y = 0.0f;
    viewport.width = (float)renderer->swapChainExtent.width;
    viewport.height = (float)renderer->swapChainExtent.height;
    viewport.minDepth = 0.0f;
    viewport.maxDepth = 1.0f;
    vkCmdSetViewport(activeCommandBuffer, 0, 1, &viewport);

    VkRect2D scissor = {};
    scissor.offset = { 0,0 };
    scissor.extent = renderer->swapChainExtent;
    vkCmdSetScissor(activeCommandBuffer, 0, 1, &scissor);

    // Bind our descriptor sets
    if (descriptorSets.size() > 0)
        vkCmdBindDescriptorSets(activeCommandBuffer,
            VK_PIPELINE_BIND_POINT_GRAPHICS,
            pipelineLayout, 0,
            1, &descriptorSets[imageIndex],
            0, NULL);

    // Bind our graphics pipeline
    vkCmdBindPipeline(activeCommandBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS, graphicsPipeline);

    // Run our renderer
    auto result = config["render"](config, worldConfig, this);
    if (!result.valid()) {
        sol::error err = result;
        Debugger::addLog(DEBUG_LEVEL_ERROR, "[LUA] " + std::string(err.what()));
        return;
    }

    // End the command buffer
    vkEndCommandBuffer(activeCommandBuffer);
    renderer->secondaryBuffers.push_back(activeCommandBuffer);
    activeCommandBuffer = VK_NULL_HANDLE;
}

void SubRenderer::windowRefresh(bool numImagesChanged, int imageCount) {
    // Frame buffers get remade every time window refreshes, so our secondary command buffer
    // inheritance info needs to be remade
    createInheritanceInfo(imageCount);

    // Update everything that only updates when numImagesChanged
    if (numImagesChanged) {
        // Command Buffers
        vkFreeCommandBuffers(*device, device->commandPool, static_cast<uint32_t>(commandBuffers.size()), commandBuffers.data());
        commandBuffers = device->createCommandBuffers(VK_COMMAND_BUFFER_LEVEL_SECONDARY, imageCount);

        // Descriptor Pool and Sets
        vkDestroyDescriptorPool(*device, descriptorPool, nullptr);
        createDescriptorPool(imageCount);
        createDescriptorSets(imageCount);
    }
}

void SubRenderer::cleanup() {
    // Destroy any textures and models
    for (auto texture : textures)
        texture->cleanup();
    for (auto model : models)
        model->cleanup();

    // Destroy our command buffers
    vkFreeCommandBuffers(*device, device->commandPool, static_cast<uint32_t>(commandBuffers.size()), commandBuffers.data());

    // Destroy our descriptor set layout
    vkDestroyDescriptorSetLayout(*device, descriptorSetLayout, nullptr);
    vkDestroyDescriptorPool(*device, descriptorPool, nullptr);

    // Destroy our graphics pipeline
    vkDestroyPipeline(*device, graphicsPipeline, nullptr);
    vkDestroyPipelineLayout(*device, pipelineLayout, nullptr);
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

    if (vkCreateDescriptorSetLayout(*device, &layoutInfo, nullptr, &descriptorSetLayout) != VK_SUCCESS) {
        throw std::runtime_error("failed to create descriptor set layout!");
    }
}

void SubRenderer::createDescriptorPool(size_t imageCount) {
    if (bindings.size() == 0) return;

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

    if (vkCreateDescriptorPool(*device, &poolInfo, nullptr, &descriptorPool) != VK_SUCCESS) {
        throw std::runtime_error("failed to create descriptor pool!");
    }
}

void SubRenderer::createDescriptorSets(size_t imageCount) {
    if (bindings.size() == 0) return;

    std::vector<VkDescriptorSetLayout> layouts(imageCount, descriptorSetLayout);
    VkDescriptorSetAllocateInfo allocInfo = {};
    allocInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_ALLOCATE_INFO;
    allocInfo.descriptorPool = descriptorPool;
    allocInfo.descriptorSetCount = static_cast<uint32_t>(imageCount);
    allocInfo.pSetLayouts = layouts.data();

    descriptorSets.resize(imageCount);
    if (vkAllocateDescriptorSets(*device, &allocInfo, descriptorSets.data()) != VK_SUCCESS) {
        throw std::runtime_error("failed to allocate descriptor sets!");
    }

    for (size_t i = 0; i < imageCount; i++) {
        std::vector<VkWriteDescriptorSet> descriptorWrites = getDescriptorWrites(descriptorSets[i]);

        vkUpdateDescriptorSets(*device, static_cast<uint32_t>(descriptorWrites.size()), descriptorWrites.data(), 0, nullptr);
    }
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
    sol::table shaders = config["shaders"];

    for (auto& kvp : shaders) {
        auto name = kvp.first.as<std::string>();
        auto stage = kvp.second.as<VkShaderStageFlagBits>();
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
    auto cullMode = config.get<sol::optional<VkCullModeFlagBits>>("cullMode");

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
    rasterizer.cullMode = cullMode.has_value() ? cullMode.value() : VK_CULL_MODE_BACK_BIT;
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
    VkBool32 performDepthTest = config.get_or("performDepthTest", true) ? VK_TRUE : VK_FALSE;

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
    uint32_t size = config.get_or("pushConstantsSize", 0);

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
    pipelineLayoutInfo.pushConstantRangeCount = (uint32_t)pushConstantRanges.size();
    pipelineLayoutInfo.pPushConstantRanges = pushConstantRanges.data();

    if (vkCreatePipelineLayout(*device, &pipelineLayoutInfo, nullptr, &pipelineLayout) != VK_SUCCESS) {
        throw std::runtime_error("failed to create pipeline layout!");
    }

    // Bring everything together for creating the actual pipeline
    VkGraphicsPipelineCreateInfo pipelineInfo = {};
    pipelineInfo.sType = VK_STRUCTURE_TYPE_GRAPHICS_PIPELINE_CREATE_INFO;
    pipelineInfo.stageCount = shaderStages.size();
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
    if (vkCreateGraphicsPipelines(*device, VK_NULL_HANDLE, 1, &pipelineInfo, nullptr, &graphicsPipeline) != VK_SUCCESS) {
        throw std::runtime_error("failed to create graphics pipeline!");
    }
}

void SubRenderer::createInheritanceInfo(size_t imageCount) {
    this->inheritanceInfo.resize(imageCount);
    for (uint32_t i = 0; i < imageCount; i++) {
        VkCommandBufferInheritanceInfo inheritanceInfo = {};
        inheritanceInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_INHERITANCE_INFO;
        inheritanceInfo.framebuffer = renderer->swapChainFramebuffers[i];
        inheritanceInfo.renderPass = renderer->renderPass;
        inheritanceInfo.subpass = 0;

        this->inheritanceInfo[i] = inheritanceInfo;
    }
}

void SubRenderer::cleanShaderModules() {
    for (auto shader : shaderModules) {
        vkDestroyShaderModule(*device, shader, nullptr);
    }
}
