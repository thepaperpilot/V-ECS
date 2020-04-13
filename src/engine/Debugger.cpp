#include "Debugger.h"
#include "../util/VulkanUtils.h"

#include <vector>
#include <iostream>

using namespace vecs;

VkDebugUtilsMessageSeverityFlagBitsEXT Debugger::logLevel = VK_DEBUG_UTILS_MESSAGE_SEVERITY_WARNING_BIT_EXT;

std::vector<Log> Debugger::log = std::vector<Log>();

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

        Debugger::log.emplace_back(DEBUG_LEVEL_ERROR, "[VULKAN] " + errorType + std::string(pCallbackData->pMessage));
    }

    return VK_FALSE;
}

void Debugger::addLog(DebugLevel debugLevel, std::string message) {
    log.emplace_back(debugLevel, message);
    std::cout << message << std::endl;
}

void Debugger::setupDebugMessenger(VkInstance instance) {
    if (!enableValidationLayers) return;

    // Register our debug messenger that prints out our validation layer's logs
    VkDebugUtilsMessengerCreateInfoEXT createInfo;
    populateDebugMessengerCreateInfo(createInfo);

    if (CreateDebugUtilsMessengerEXT(instance, &createInfo, nullptr, &debugMessenger) != VK_SUCCESS) {
        throw std::runtime_error("failed to set up debug messenger!");
    }
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
