#include "Debugger.h"

#include "../util/VulkanUtils.h"

#include <GLFW\glfw3.h>

#include <vector>
#include <iostream>
#include <mutex>

using namespace vecs;

VkDebugUtilsMessageSeverityFlagBitsEXT Debugger::logLevel = VK_DEBUG_UTILS_MESSAGE_SEVERITY_WARNING_BIT_EXT;

std::vector<Log> Debugger::log = std::vector<Log>();

std::mutex debugMutex;

// TODO look at the Vulkan debug docs to make this more robust
// https://www.khronos.org/registry/vulkan/specs/1.1-extensions/html/vkspec.html#VK_EXT_debug_utils
VKAPI_ATTR VkBool32 VKAPI_CALL Debugger::debugCallback(
    VkDebugUtilsMessageSeverityFlagBitsEXT messageSeverity,
    VkDebugUtilsMessageTypeFlagsEXT messageType,
    const VkDebugUtilsMessengerCallbackDataEXT* pCallbackData,
    void* pUserData) {

    if (messageSeverity >= logLevel) {
        std::string errorType =
            messageType == VK_DEBUG_UTILS_MESSAGE_TYPE_GENERAL_BIT_EXT ? "[GENERAL] " :
            messageType == VK_DEBUG_UTILS_MESSAGE_TYPE_VALIDATION_BIT_EXT ? "[VALIDATION] " :
            "[PERFORMANCE] ";

        Debugger::addLog(DEBUG_LEVEL_ERROR, "[VULKAN] " + errorType + std::string(pCallbackData->pMessage));
    }

    return VK_FALSE;
}

void Debugger::setupLogFile(std::string filename) {
//#ifdef NDEBUG
    freopen(filename.c_str(), "w", stdout);
//#endif
}

void Debugger::addLog(DebugLevel debugLevel, std::string message) {
    debugMutex.lock();
    log.emplace_back(debugLevel, message);
    std::cout << glfwGetTime() << " " << message << std::endl;
    debugMutex.unlock();
}

std::string Debugger::getErrorString(VkResult errorCode) {
    switch (errorCode)
    {
#define STR(r) case VK_ ##r: return #r
        STR(NOT_READY);
        STR(TIMEOUT);
        STR(EVENT_SET);
        STR(EVENT_RESET);
        STR(INCOMPLETE);
        STR(ERROR_OUT_OF_HOST_MEMORY);
        STR(ERROR_OUT_OF_DEVICE_MEMORY);
        STR(ERROR_INITIALIZATION_FAILED);
        STR(ERROR_DEVICE_LOST);
        STR(ERROR_MEMORY_MAP_FAILED);
        STR(ERROR_LAYER_NOT_PRESENT);
        STR(ERROR_EXTENSION_NOT_PRESENT);
        STR(ERROR_FEATURE_NOT_PRESENT);
        STR(ERROR_INCOMPATIBLE_DRIVER);
        STR(ERROR_TOO_MANY_OBJECTS);
        STR(ERROR_FORMAT_NOT_SUPPORTED);
        STR(ERROR_SURFACE_LOST_KHR);
        STR(ERROR_NATIVE_WINDOW_IN_USE_KHR);
        STR(SUBOPTIMAL_KHR);
        STR(ERROR_OUT_OF_DATE_KHR);
        STR(ERROR_INCOMPATIBLE_DISPLAY_KHR);
        STR(ERROR_VALIDATION_FAILED_EXT);
        STR(ERROR_INVALID_SHADER_NV);
#undef STR
    default:
        return "UNKNOWN_ERROR";
    }
}

void Debugger::setupDebugMessenger(VkInstance instance) {
    if (!enableDebugMessenger) return;

    // Register our debug messenger that prints out our validation layer's logs
    VkDebugUtilsMessengerCreateInfoEXT createInfo;
    populateDebugMessengerCreateInfo(createInfo);

    VK_CHECK_RESULT(CreateDebugUtilsMessengerEXT(instance, &createInfo, nullptr, &debugMessenger));
}

bool Debugger::checkValidationLayerSupport() {
    // If we don't have validation layers enabled, then there's none to be not support
    if (!enableValidationLayers) return true;

    // Find out how many available layers we have
    uint32_t layerCount;
    vkEnumerateInstanceLayerProperties(&layerCount, nullptr);

    // Store the data on the available layers
    std::vector<VkLayerProperties> availableLayers(layerCount);
    vkEnumerateInstanceLayerProperties(&layerCount, availableLayers.data());

    // Check all our required validation layers are, in fact, available
    // Returning false if we require one that isn't available
    // Iterate through each layer we need
    for (const char* layerName : validationLayers) {
        bool layerFound = false;

        // Iterate through each layer we have available
        for (const auto& layerProperties : availableLayers) {
            if (strcmp(layerName, layerProperties.layerName) == 0) {
                // If the available layer has the same name as the one we require,
                // flip a flag saying we found what we're looking for, and break
                // out of this loop as a small optimization
                layerFound = true;
                break;
            }
        }

        // Return false if one of our required layers wasn't found
        if (!layerFound) {
            std::cout << "Could not find layer " << layerName << std::endl;
            return false;
        }
    }

    // This'll run if every required layer was found amongst our available layers
    return true;
}

void Debugger::populateDebugMessengerCreateInfo(VkDebugUtilsMessengerCreateInfoEXT& createInfo) {
    createInfo = {};
    createInfo.sType = VK_STRUCTURE_TYPE_DEBUG_UTILS_MESSENGER_CREATE_INFO_EXT;
    // We don't filter here because we'd rather filter when we actually receive the message
    // Note that this won't make the release version any slower, because we only create
    // the debug messenger in Debug mode
    createInfo.messageSeverity =
        VK_DEBUG_UTILS_MESSAGE_SEVERITY_VERBOSE_BIT_EXT |
        VK_DEBUG_UTILS_MESSAGE_SEVERITY_INFO_BIT_EXT |
        VK_DEBUG_UTILS_MESSAGE_SEVERITY_WARNING_BIT_EXT |
        VK_DEBUG_UTILS_MESSAGE_SEVERITY_ERROR_BIT_EXT;
    createInfo.messageType = VK_DEBUG_UTILS_MESSAGE_TYPE_GENERAL_BIT_EXT |
        VK_DEBUG_UTILS_MESSAGE_TYPE_VALIDATION_BIT_EXT |
        VK_DEBUG_UTILS_MESSAGE_TYPE_PERFORMANCE_BIT_EXT;
    createInfo.pfnUserCallback = debugCallback;
    // I can use this if I ever need to pass custom info to the debug callback
    createInfo.pUserData = nullptr;
}

void Debugger::cleanup(VkInstance instance) {
    DestroyDebugUtilsMessengerEXT(instance, debugMessenger, nullptr);
}
