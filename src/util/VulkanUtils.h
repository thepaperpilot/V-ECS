#pragma once

#include "../engine/Debugger.h"

#include <vulkan/vulkan.h>

#include <glslang/Public/ShaderLang.h>
#include <SPIRV/GlslangToSpv.h>
// The hunter package didn't include this and I don't know how to change that
// so I've included the file by hand in this directory, just modifying the #include statement
//#include <StandAlone/DirStackFileIncluder.h>
#include "DirStackFileIncluder.h"

#include <filesystem>
#include <vector>
#include <fstream>
#include <iostream>
#include <cassert>

using namespace vecs;

static VkResult CreateDebugUtilsMessengerEXT(
    VkInstance instance,
    const VkDebugUtilsMessengerCreateInfoEXT* pCreateInfo,
    const VkAllocationCallbacks* pAllocator,
    VkDebugUtilsMessengerEXT* pDebugMessenger) {

    auto func = (PFN_vkCreateDebugUtilsMessengerEXT)vkGetInstanceProcAddr(instance, "vkCreateDebugUtilsMessengerEXT");
    if (func != nullptr) {
        return func(instance, pCreateInfo, pAllocator, pDebugMessenger);
    } else {
        return VK_ERROR_EXTENSION_NOT_PRESENT;
    }
}

static void DestroyDebugUtilsMessengerEXT(
    VkInstance instance,
    VkDebugUtilsMessengerEXT debugMessenger,
    const VkAllocationCallbacks* pAllocator) {

    auto func = (PFN_vkDestroyDebugUtilsMessengerEXT)vkGetInstanceProcAddr(instance, "vkDestroyDebugUtilsMessengerEXT");
    if (func != nullptr) {
        func(instance, debugMessenger, pAllocator);
    }
}

static bool glslangInitialized = false;

static EShLanguage GetShaderStage(const std::filesystem::path& extension) {
    if (extension == ".vert") {
        return EShLangVertex;
    }
    else if (extension == ".tesc") {
        return EShLangTessControl;
    }
    else if (extension == ".tese") {
        return EShLangTessEvaluation;
    }
    else if (extension == ".geom") {
        return EShLangGeometry;
    }
    else if (extension == ".frag") {
        return EShLangFragment;
    }
    else if (extension == ".comp") {
        return EShLangCompute;
    }
    else {
        assert(0 && "Unknown shader stage");
        return EShLangCount;
    }
}

// Copied from https://github.com/KhronosGroup/glslang/blob/master/StandAlone/ResourceLimits.cpp
static const TBuiltInResource DefaultTBuiltInResource = {
    /* .MaxLights = */ 32,
    /* .MaxClipPlanes = */ 6,
    /* .MaxTextureUnits = */ 32,
    /* .MaxTextureCoords = */ 32,
    /* .MaxVertexAttribs = */ 64,
    /* .MaxVertexUniformComponents = */ 4096,
    /* .MaxVaryingFloats = */ 64,
    /* .MaxVertexTextureImageUnits = */ 32,
    /* .MaxCombinedTextureImageUnits = */ 80,
    /* .MaxTextureImageUnits = */ 32,
    /* .MaxFragmentUniformComponents = */ 4096,
    /* .MaxDrawBuffers = */ 32,
    /* .MaxVertexUniformVectors = */ 128,
    /* .MaxVaryingVectors = */ 8,
    /* .MaxFragmentUniformVectors = */ 16,
    /* .MaxVertexOutputVectors = */ 16,
    /* .MaxFragmentInputVectors = */ 15,
    /* .MinProgramTexelOffset = */ -8,
    /* .MaxProgramTexelOffset = */ 7,
    /* .MaxClipDistances = */ 8,
    /* .MaxComputeWorkGroupCountX = */ 65535,
    /* .MaxComputeWorkGroupCountY = */ 65535,
    /* .MaxComputeWorkGroupCountZ = */ 65535,
    /* .MaxComputeWorkGroupSizeX = */ 1024,
    /* .MaxComputeWorkGroupSizeY = */ 1024,
    /* .MaxComputeWorkGroupSizeZ = */ 64,
    /* .MaxComputeUniformComponents = */ 1024,
    /* .MaxComputeTextureImageUnits = */ 16,
    /* .MaxComputeImageUniforms = */ 8,
    /* .MaxComputeAtomicCounters = */ 8,
    /* .MaxComputeAtomicCounterBuffers = */ 1,
    /* .MaxVaryingComponents = */ 60,
    /* .MaxVertexOutputComponents = */ 64,
    /* .MaxGeometryInputComponents = */ 64,
    /* .MaxGeometryOutputComponents = */ 128,
    /* .MaxFragmentInputComponents = */ 128,
    /* .MaxImageUnits = */ 8,
    /* .MaxCombinedImageUnitsAndFragmentOutputs = */ 8,
    /* .MaxCombinedShaderOutputResources = */ 8,
    /* .MaxImageSamples = */ 0,
    /* .MaxVertexImageUniforms = */ 0,
    /* .MaxTessControlImageUniforms = */ 0,
    /* .MaxTessEvaluationImageUniforms = */ 0,
    /* .MaxGeometryImageUniforms = */ 0,
    /* .MaxFragmentImageUniforms = */ 8,
    /* .MaxCombinedImageUniforms = */ 8,
    /* .MaxGeometryTextureImageUnits = */ 16,
    /* .MaxGeometryOutputVertices = */ 256,
    /* .MaxGeometryTotalOutputComponents = */ 1024,
    /* .MaxGeometryUniformComponents = */ 1024,
    /* .MaxGeometryVaryingComponents = */ 64,
    /* .MaxTessControlInputComponents = */ 128,
    /* .MaxTessControlOutputComponents = */ 128,
    /* .MaxTessControlTextureImageUnits = */ 16,
    /* .MaxTessControlUniformComponents = */ 1024,
    /* .MaxTessControlTotalOutputComponents = */ 4096,
    /* .MaxTessEvaluationInputComponents = */ 128,
    /* .MaxTessEvaluationOutputComponents = */ 128,
    /* .MaxTessEvaluationTextureImageUnits = */ 16,
    /* .MaxTessEvaluationUniformComponents = */ 1024,
    /* .MaxTessPatchComponents = */ 120,
    /* .MaxPatchVertices = */ 32,
    /* .MaxTessGenLevel = */ 64,
    /* .MaxViewports = */ 16,
    /* .MaxVertexAtomicCounters = */ 0,
    /* .MaxTessControlAtomicCounters = */ 0,
    /* .MaxTessEvaluationAtomicCounters = */ 0,
    /* .MaxGeometryAtomicCounters = */ 0,
    /* .MaxFragmentAtomicCounters = */ 8,
    /* .MaxCombinedAtomicCounters = */ 8,
    /* .MaxAtomicCounterBindings = */ 1,
    /* .MaxVertexAtomicCounterBuffers = */ 0,
    /* .MaxTessControlAtomicCounterBuffers = */ 0,
    /* .MaxTessEvaluationAtomicCounterBuffers = */ 0,
    /* .MaxGeometryAtomicCounterBuffers = */ 0,
    /* .MaxFragmentAtomicCounterBuffers = */ 1,
    /* .MaxCombinedAtomicCounterBuffers = */ 1,
    /* .MaxAtomicCounterBufferSize = */ 16384,
    /* .MaxTransformFeedbackBuffers = */ 4,
    /* .MaxTransformFeedbackInterleavedComponents = */ 64,
    /* .MaxCullDistances = */ 8,
    /* .MaxCombinedClipAndCullDistances = */ 8,
    /* .MaxSamples = */ 4,
    /* .maxMeshOutputVerticesNV = */ 256,
    /* .maxMeshOutputPrimitivesNV = */ 512,
    /* .maxMeshWorkGroupSizeX_NV = */ 32,
    /* .maxMeshWorkGroupSizeY_NV = */ 1,
    /* .maxMeshWorkGroupSizeZ_NV = */ 1,
    /* .maxTaskWorkGroupSizeX_NV = */ 32,
    /* .maxTaskWorkGroupSizeY_NV = */ 1,
    /* .maxTaskWorkGroupSizeZ_NV = */ 1,
    /* .maxMeshViewCountNV = */ 4,
    /* .maxDualSourceDrawBuffersEXT = */ 1,

    /* .limits = */ {
    /* .nonInductiveForLoops = */ 1,
    /* .whileLoops = */ 1,
    /* .doWhileLoops = */ 1,
    /* .generalUniformIndexing = */ 1,
    /* .generalAttributeMatrixVectorIndexing = */ 1,
    /* .generalVaryingIndexing = */ 1,
    /* .generalSamplerIndexing = */ 1,
    /* .generalVariableIndexing = */ 1,
    /* .generalConstantMatrixVectorIndexing = */ 1,
} };

static std::vector<char> readFile(const std::string& filename) {
    std::ifstream file(filename, std::ios::ate | std::ios::binary);

    if (!file.is_open()) {
        throw std::runtime_error("failed to open file!");
    }

    size_t fileSize = (size_t)file.tellg();
    std::vector<char> buffer(fileSize);

    file.seekg(0);
    file.read(buffer.data(), fileSize);

    file.close();

    return buffer;
}

// Reference used: https://github.com/ForestCSharp/VkCppRenderer/blob/master/Src/Renderer/GLSL/ShaderCompiler.hpp
// TODO validate shader and output errors
static const VkShaderModule getCompiledShader(VkDevice* device, const std::string& filename) {
    auto filepath = std::filesystem::path(filename).make_preferred();
    // If the source code hasn't been modified since we last compiled our shader, just use the already compiled one.
    auto compiledFilepath = std::filesystem::path(filepath.string() + ".spv");

    VkShaderModule shaderModule;
    VkShaderModuleCreateInfo createInfo = {};
    createInfo.sType = VK_STRUCTURE_TYPE_SHADER_MODULE_CREATE_INFO;

    if (std::filesystem::exists(compiledFilepath) &&
        std::filesystem::last_write_time(compiledFilepath) >= std::filesystem::last_write_time(filepath)) {

        auto code = readFile(compiledFilepath.string());
        createInfo.codeSize = code.size();
        createInfo.pCode = reinterpret_cast<const uint32_t*>(code.data());

        // Create the shader module
        VK_CHECK_RESULT(vkCreateShaderModule(*device, &createInfo, nullptr, &shaderModule));
    } else {
        // from source: "ShInitialize() should be called exactly once per process, not per thread."
        if (!glslangInitialized) {
            glslang::InitializeProcess();
            glslangInitialized = true;
        }

        //Load GLSL into a string
        std::ifstream file(filename);

        if (!file.is_open()) {
            throw std::runtime_error("failed to open shader: " + filename);
        }

        std::string InputGLSL((std::istreambuf_iterator<char>(file)),
            std::istreambuf_iterator<char>());

        const char* InputCString = InputGLSL.c_str();
        EShLanguage ShaderType = GetShaderStage(filepath.extension());
        glslang::TShader Shader(ShaderType);
        Shader.setStrings(&InputCString, 1);

        //Set up Vulkan/SpirV Environment
        int ClientInputSemanticsVersion = 100; // maps to, say, #define VULKAN 100
        glslang::EShTargetClientVersion VulkanClientVersion = glslang::EShTargetVulkan_1_0;  // would map to, say, Vulkan 1.0
        glslang::EShTargetLanguageVersion TargetVersion = glslang::EShTargetSpv_1_0;    // maps to, say, SPIR-V 1.0

        Shader.setEnvInput(glslang::EShSourceGlsl, ShaderType, glslang::EShClientVulkan, ClientInputSemanticsVersion);
        Shader.setEnvClient(glslang::EShClientVulkan, VulkanClientVersion);
        Shader.setEnvTarget(glslang::EShTargetSpv, TargetVersion);

        TBuiltInResource Resources;
        Resources = DefaultTBuiltInResource;
        EShMessages messages = (EShMessages)(EShMsgSpvRules | EShMsgVulkanRules);

        const int DefaultVersion = 100;

        DirStackFileIncluder Includer;
        Includer.pushExternalLocalDirectory(filepath.parent_path().string());

        std::string PreprocessedGLSL;

        if (!Shader.preprocess(&Resources, DefaultVersion, ENoProfile, false, false, messages, &PreprocessedGLSL, Includer)) {
            Debugger::addLog(DEBUG_LEVEL_ERROR, "[SHADER] " + filepath.string() + " failed GLSL Preprocessing step. " + Shader.getInfoLog() + "\n" + Shader.getInfoDebugLog());
        }

        const char* PreprocessedCStr = PreprocessedGLSL.c_str();
        Shader.setStrings(&PreprocessedCStr, 1);

        if (!Shader.parse(&Resources, 100, false, messages)) {
            Debugger::addLog(DEBUG_LEVEL_ERROR, "[SHADER] " + filepath.string() + " failed GLSL Parsing step. " + Shader.getInfoLog() + "\n" + Shader.getInfoDebugLog());
        }

        glslang::TProgram Program;
        Program.addShader(&Shader);

        if (!Program.link(messages)) {
            Debugger::addLog(DEBUG_LEVEL_ERROR, "[SHADER] " + filepath.string() + " failed GLSL Linking step. " + Shader.getInfoLog() + "\n" + Shader.getInfoDebugLog());
        }

        std::vector<unsigned int> SpirV;
        spv::SpvBuildLogger logger;
        glslang::SpvOptions spvOptions;
        glslang::GlslangToSpv(*Program.getIntermediate(ShaderType), SpirV, &logger, &spvOptions);

        glslang::OutputSpvBin(SpirV, compiledFilepath.string().c_str());

        if (logger.getAllMessages().length() > 0) {
            Debugger::addLog(DEBUG_LEVEL_WARN, "[SHADER] " + filepath.string() + " compiled with the following output: " + logger.getAllMessages());
        }

        //TODO: Handle startup shutdown separately from compile function
        //glslang::FinalizeProcess();
        createInfo.codeSize = SpirV.size() * sizeof(unsigned int);
        createInfo.pCode = reinterpret_cast<const uint32_t*>(SpirV.data());

        // Create the shader module
        VK_CHECK_RESULT(vkCreateShaderModule(*device, &createInfo, nullptr, &shaderModule));
    }

    return shaderModule;
}
