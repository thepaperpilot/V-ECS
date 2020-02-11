#include "SubRenderer.h"
#include "../engine/Device.h"
#include "../util/VulkanUtils.h"

#include <vulkan/vulkan.h>

using namespace vecs;

void SubRenderer::init(Device* device, Renderer* renderer, VkRenderPass* renderPass) {
    this->device = device;
    this->renderer = renderer;
    this->renderPass = renderPass;

    createDescriptorSetLayout();

    shaderStages = getShaderModules();
    vertexInputInfo = getVertexInputInfo();
    inputAssembly = getInputAssembly();
    viewportState = getViewportState();
    rasterizer = getRasterizer();
    multisampling = getMultisampling();
    depthStencil = getDepthStencil();
    colorBlending = getColorBlending();
    dynamicState = getDynamicState();
    pushConstantRanges = getPushConstantRanges();

    createGraphicsPipeline();
}

void SubRenderer::markAllBuffersDirty() {
    for (int i = commandBuffers.size() - 1; i >= 0; i--) {
        dirtyBuffers.emplace(static_cast<uint32_t>(i));
    }
}

void SubRenderer::buildCommandBuffer(uint32_t imageIndex) {
    // Begin the command buffer
    device->beginCommandBuffer(commandBuffers[imageIndex]);

    // Bind our descriptor sets
    vkCmdBindDescriptorSets(commandBuffers[imageIndex],
        VK_PIPELINE_BIND_POINT_GRAPHICS,
        pipelineLayout, 0,
        descriptorSets.size(), descriptorSets.data(),
        0, NULL);

    // Bind our graphics pipeline
    vkCmdBindPipeline(commandBuffers[imageIndex], VK_PIPELINE_BIND_POINT_GRAPHICS, graphicsPipeline);

    // Draw our geometry
    render(commandBuffers[imageIndex]);

    // End the command buffer
    vkEndCommandBuffer(commandBuffers[imageIndex]);

    // Mark this buffer not dirty
    dirtyBuffers.erase(imageIndex);
}

void SubRenderer::windowRefresh(bool numImagesChanged, size_t imageCount) {
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

    OnWindowRefresh(numImagesChanged, imageCount);
}

void SubRenderer::cleanup() {
    // Destroy our command buffers
    vkFreeCommandBuffers(*device, device->commandPool, static_cast<uint32_t>(commandBuffers.size()), commandBuffers.data());

    // Destroy our descriptor set layout
    vkDestroyDescriptorSetLayout(*device, descriptorSetLayout, nullptr);

    // Destroy our graphics pipeline
    vkDestroyPipeline(*device, graphicsPipeline, nullptr);
    vkDestroyPipelineLayout(*device, pipelineLayout, nullptr);
}

std::array<VkPipelineShaderStageCreateInfo, 2> SubRenderer::getShaderModules() {
    // Read our compiled shaders from their files
    // Note they are in SPIR-V format (compiled from GLSL)
    auto vertShaderCode = readFile(vertShaderFile);
    auto fragShaderCode = readFile(fragShaderFile);

    // Wrap the shader bytecode into a shader module
    vertShaderModule = createShaderModule(vertShaderCode);
    fragShaderModule = createShaderModule(fragShaderCode);

    // Create our vertex shader stage info
    VkPipelineShaderStageCreateInfo vertShaderStageInfo = {};
    vertShaderStageInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO;
    vertShaderStageInfo.stage = VK_SHADER_STAGE_VERTEX_BIT;
    vertShaderStageInfo.module = vertShaderModule;
    vertShaderStageInfo.pName = "main";

    // Create our fragment shader stage info
    VkPipelineShaderStageCreateInfo fragShaderStageInfo = {};
    fragShaderStageInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO;
    fragShaderStageInfo.stage = VK_SHADER_STAGE_FRAGMENT_BIT;
    fragShaderStageInfo.module = fragShaderModule;
    fragShaderStageInfo.pName = "main";

    return { vertShaderStageInfo, fragShaderStageInfo };
}

VkPipelineVertexInputStateCreateInfo SubRenderer::getVertexInputInfo() {
    // Describe the format of the vertex data being passed to the vertex shader
    VkPipelineVertexInputStateCreateInfo vertexInputInfo = {};
    auto bindingDescription = getBindingDescription();
    attributeDescriptions = getAttributeDescriptions();
    vertexInputInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_VERTEX_INPUT_STATE_CREATE_INFO;
    vertexInputInfo.vertexBindingDescriptionCount = 1;
    vertexInputInfo.vertexAttributeDescriptionCount = static_cast<uint32_t>(attributeDescriptions.size());
    vertexInputInfo.pVertexBindingDescriptions = &bindingDescription;
    vertexInputInfo.pVertexAttributeDescriptions = attributeDescriptions.data();
    return vertexInputInfo;
}

VkPipelineInputAssemblyStateCreateInfo SubRenderer::getInputAssembly() {
    // Describe what kind of geometry will be drawn from the vertices
    // We're telling it to not reuse vertices, but this is where you'd change that
    // e.g. each triangle could reuse the last two vertices from the previous triangle
    // But we're just going to send all 3 vertices for each and every triangle to be drawn
    // Using an index buffer to optimize everything
    VkPipelineInputAssemblyStateCreateInfo inputAssembly = {};
    inputAssembly.sType = VK_STRUCTURE_TYPE_PIPELINE_INPUT_ASSEMBLY_STATE_CREATE_INFO;
    inputAssembly.topology = VK_PRIMITIVE_TOPOLOGY_TRIANGLE_LIST;
    inputAssembly.primitiveRestartEnable = VK_FALSE;

    return inputAssembly;
}

VkPipelineViewportStateCreateInfo SubRenderer::getViewportState() {
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

    return viewportState;
}

VkPipelineRasterizationStateCreateInfo SubRenderer::getRasterizer() {
    // Describe our rasterizer
    VkPipelineRasterizationStateCreateInfo rasterizer = {};
    rasterizer.sType = VK_STRUCTURE_TYPE_PIPELINE_RASTERIZATION_STATE_CREATE_INFO;
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
    // These two options determine how to calculate culling
    rasterizer.cullMode = VK_CULL_MODE_BACK_BIT;
    rasterizer.frontFace = VK_FRONT_FACE_CLOCKWISE;
    // This is a setting that can help with shadow mapping
    rasterizer.depthBiasEnable = VK_FALSE;
    rasterizer.cullMode = VK_CULL_MODE_BACK_BIT;
    rasterizer.frontFace = VK_FRONT_FACE_COUNTER_CLOCKWISE;

    return rasterizer;
}

VkPipelineMultisampleStateCreateInfo SubRenderer::getMultisampling() {
    // Describe our multisampling AA
    VkPipelineMultisampleStateCreateInfo multisampling = {};
    multisampling.sType = VK_STRUCTURE_TYPE_PIPELINE_MULTISAMPLE_STATE_CREATE_INFO;
    multisampling.sampleShadingEnable = VK_FALSE;
    multisampling.rasterizationSamples = VK_SAMPLE_COUNT_1_BIT;

    return multisampling;
}

VkPipelineDepthStencilStateCreateInfo SubRenderer::getDepthStencil() {
    // Describe the depth and stencil buffer
    VkPipelineDepthStencilStateCreateInfo depthStencil = {};
    depthStencil.sType = VK_STRUCTURE_TYPE_PIPELINE_DEPTH_STENCIL_STATE_CREATE_INFO;
    depthStencil.depthTestEnable = VK_TRUE;
    depthStencil.depthWriteEnable = VK_TRUE;
    depthStencil.depthCompareOp = VK_COMPARE_OP_LESS;
    depthStencil.depthBoundsTestEnable = VK_FALSE;
    depthStencil.stencilTestEnable = VK_FALSE;

    return depthStencil;
}

VkPipelineColorBlendStateCreateInfo SubRenderer::getColorBlending() {
    // Describe color blending
    // First part is for our frame buffers, of which we only have one
    VkPipelineColorBlendAttachmentState colorBlendAttachment = {};
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
    VkPipelineColorBlendStateCreateInfo colorBlending = {};
    colorBlending.sType = VK_STRUCTURE_TYPE_PIPELINE_COLOR_BLEND_STATE_CREATE_INFO;
    // Setting this to true will ignore all the frame buffer-specific color blending
    colorBlending.logicOpEnable = VK_FALSE;
    colorBlending.attachmentCount = 1;
    colorBlending.pAttachments = &colorBlendAttachment;

    return colorBlending;
}

VkPipelineDynamicStateCreateInfo SubRenderer::getDynamicState() {
    // Specify what properties we want to specify at draw-time
    VkDynamicState dynamicStates[] = {
        VK_DYNAMIC_STATE_VIEWPORT,
        VK_DYNAMIC_STATE_SCISSOR
    };
    VkPipelineDynamicStateCreateInfo dynamicState = {};
    dynamicState.sType = VK_STRUCTURE_TYPE_PIPELINE_DYNAMIC_STATE_CREATE_INFO;
    dynamicState.dynamicStateCount = 2;
    dynamicState.pDynamicStates = dynamicStates;

    return dynamicState;
}

void SubRenderer::cleanShaderModules() {
    vkDestroyShaderModule(*device, fragShaderModule, nullptr);
    vkDestroyShaderModule(*device, vertShaderModule, nullptr);
}

VkShaderModule SubRenderer::createShaderModule(const std::vector<char>& code) {
    // Create the info for this shader module
    // It needs the size of the bytecode and a pointer to the bytecode (no need for null termination)
    VkShaderModuleCreateInfo createInfo = {};
    createInfo.sType = VK_STRUCTURE_TYPE_SHADER_MODULE_CREATE_INFO;
    createInfo.codeSize = code.size();
    createInfo.pCode = reinterpret_cast<const uint32_t*>(code.data());

    // Create the shader module
    VkShaderModule shaderModule;
    if (vkCreateShaderModule(*device, &createInfo, nullptr, &shaderModule) != VK_SUCCESS) {
        throw std::runtime_error("failed to create shader module!");
    }

    return shaderModule;
}

void SubRenderer::createDescriptorSetLayout() {
    // Ignore them for now, because we don't have any to use
    bindings = getDescriptorLayoutBindings();
    VkDescriptorSetLayoutCreateInfo layoutInfo = {};
    layoutInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_CREATE_INFO;
    layoutInfo.bindingCount = static_cast<uint32_t>(bindings.size());
    layoutInfo.pBindings = bindings.data();

    if (vkCreateDescriptorSetLayout(*device, &layoutInfo, nullptr, &descriptorSetLayout) != VK_SUCCESS) {
        throw std::runtime_error("failed to create descriptor set layout!");
    }
}

void SubRenderer::createGraphicsPipeline() {
    // Save our pipeline layout
    // We add our descriptor set and push constant ranges here
    VkPipelineLayoutCreateInfo pipelineLayoutInfo = {};
    pipelineLayoutInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_LAYOUT_CREATE_INFO;
    pipelineLayoutInfo.setLayoutCount = 1;
    pipelineLayoutInfo.pSetLayouts = &descriptorSetLayout;
    pipelineLayoutInfo.pushConstantRangeCount = pushConstantRanges.size();
    pipelineLayoutInfo.pPushConstantRanges = pushConstantRanges.data();

    if (vkCreatePipelineLayout(*device, &pipelineLayoutInfo, nullptr, &pipelineLayout) != VK_SUCCESS) {
        throw std::runtime_error("failed to create pipeline layout!");
    }

    // Bring everything together for creating the actual pipeline
    VkGraphicsPipelineCreateInfo pipelineInfo = {};
    pipelineInfo.sType = VK_STRUCTURE_TYPE_GRAPHICS_PIPELINE_CREATE_INFO;
    pipelineInfo.stageCount = 2;
    pipelineInfo.pStages = getShaderModules().data();
    pipelineInfo.pVertexInputState = &getVertexInputInfo();
    pipelineInfo.pInputAssemblyState = &getInputAssembly();
    pipelineInfo.pViewportState = &getViewportState();
    pipelineInfo.pRasterizationState = &getRasterizer();
    pipelineInfo.pMultisampleState = &getMultisampling();
    pipelineInfo.pDepthStencilState = &getDepthStencil();
    pipelineInfo.pColorBlendState = &getColorBlending();
    pipelineInfo.layout = pipelineLayout;
    pipelineInfo.renderPass = *renderPass;
    pipelineInfo.subpass = 0;

    // Create the graphics pipeline!
    if (vkCreateGraphicsPipelines(*device, VK_NULL_HANDLE, 1, &pipelineInfo, nullptr, &graphicsPipeline) != VK_SUCCESS) {
        throw std::runtime_error("failed to create graphics pipeline!");
    }

    // Cleanup our shader modules
    cleanShaderModules();
}

void SubRenderer::createDescriptorPool(size_t imageCount) {
    std::vector<VkDescriptorPoolSize> poolSizes(bindings.size());
    for (int i = bindings.size() - 1; i >= 0; i--) {
        poolSizes[i].type = bindings[i].descriptorType;
        poolSizes[i].descriptorCount = static_cast<uint32_t>(imageCount);
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
