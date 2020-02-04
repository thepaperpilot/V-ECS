#include "../engine/Engine.h"
#include "../util/VulkanUtils.h"
#include "../events/EventManager.h"
#include "../events/WindowResizeEvent.h"

#include <map>
#include <set>

using namespace vecs;

// List of validation layers to use in debug mode
// These will check our code to make sure it doesn't get into an invalid state
const std::vector<const char*> validationLayers = {
    "VK_LAYER_KHRONOS_validation"
};

// List of physical device extensions that must be supported for a physical
// device to be considered at all suitable
const std::vector<const char*> deviceExtensions = {
    VK_KHR_SWAPCHAIN_EXTENSION_NAME
};

#ifdef NDEBUG
const bool enableValidationLayers = false;
#else
const bool enableValidationLayers = true;
#endif

VkDebugUtilsMessageSeverityFlagBitsEXT Engine::logLevel = VK_DEBUG_UTILS_MESSAGE_SEVERITY_WARNING_BIT_EXT;

// TODO look at the Vulkan debug docs to make this more robust
// https://www.khronos.org/registry/vulkan/specs/1.1-extensions/html/vkspec.html#VK_EXT_debug_utils
VKAPI_ATTR VkBool32 VKAPI_CALL Engine::debugCallback(
    VkDebugUtilsMessageSeverityFlagBitsEXT messageSeverity,
    VkDebugUtilsMessageTypeFlagsEXT messageType,
    const VkDebugUtilsMessengerCallbackDataEXT* pCallbackData,
    void* pUserData) {

    if (messageSeverity >= logLevel) {
        const char* errorType =
            messageType == VK_DEBUG_UTILS_MESSAGE_TYPE_GENERAL_BIT_EXT ? "GENERAL - " :
            messageType == VK_DEBUG_UTILS_MESSAGE_TYPE_VALIDATION_BIT_EXT ? "VALIDATION - " :
            "PERFORMANCE - ";

        std::cout << "[VULKAN] " << errorType << pCallbackData->pMessage << std::endl;
    }

    return VK_FALSE;
}

void Engine::windowResizeCallback(GLFWwindow* window, int width, int height) {
    // TODO pause game if not paused already, because
    // ECS world won't update while resizing on some systems
    // except for frames where window size actually changed
    WindowResizeEvent event;
    event.width = width;
    event.height = height;
    EventManager::fire(event);
}

void Engine::run(World* initialWorld) {
    renderer.engine = this;

    initWindow();
    initVulkan();
    setupWorld(initialWorld);
    mainLoop();
    cleanup();
}

void Engine::initWindow() {
    glfwInit();

    glfwWindowHint(GLFW_CLIENT_API, GLFW_NO_API);
    glfwWindowHint(GLFW_RESIZABLE, GLFW_TRUE);

    window = glfwCreateWindow(windowWidth, windowHeight, applicationName, nullptr, nullptr);

    // Register GLFW callbacks
    glfwSetWindowUserPointer(window, this);
    glfwSetWindowSizeLimits(window, 1, 1, GLFW_DONT_CARE, GLFW_DONT_CARE);
    glfwSetFramebufferSizeCallback(window, windowResizeCallback);
    glfwSetWindowRefreshCallback(window, [](GLFWwindow* window) { EventManager::fire(RefreshWindowEvent()); });
}

void Engine::initVulkan() {
    createInstance();
    setupDebugMessenger();
    createSurface();

    PhysicalDeviceCandidate candidate = pickPhysicalDevice();
    createLogicalDevice(candidate.indices);
    renderer.init(candidate.indices, candidate.swapChainSupport);
}

void Engine::createInstance() {
    // If we're in Debug mode, ensure we have validation layer support
    // If we don't, give a warning so the developer is made aware they'll
    // need to download them (should be included in the Vulkan SDK)
    if (enableValidationLayers && !checkValidationLayerSupport()) {
        throw std::runtime_error("validation layers requested, but not available!");
    }

    // Create our application info. Confiures the version, name, etc.
    // of the application (defined by the developer using the engine)
    // as well as the engine version, name, etc. (hardcoded)
    VkApplicationInfo appInfo = {};
    appInfo.sType = VK_STRUCTURE_TYPE_APPLICATION_INFO;
    appInfo.pApplicationName = applicationName;
    appInfo.applicationVersion = applicationVersion || VK_MAKE_VERSION(1, 0, 0);
    appInfo.pEngineName = "Voxel-ECS";
    appInfo.engineVersion = VK_MAKE_VERSION(1, 0, 0);
    appInfo.apiVersion = VK_API_VERSION_1_0;

    // Create our Instance Creation info. Most important part is that we
    // attach our application info we just created to this.
    VkInstanceCreateInfo createInfo = {};
    createInfo.sType = VK_STRUCTURE_TYPE_INSTANCE_CREATE_INFO;
    createInfo.pApplicationInfo = &appInfo;

    // Ensure we have the extensions we need and configure our createInfo
    // correspondingly
    auto extensions = getRequiredExtensions();
    createInfo.enabledExtensionCount = static_cast<uint32_t>(extensions.size());
    createInfo.ppEnabledExtensionNames = extensions.data();

    // If we're in debug mode, add our validation layers to createInfo
    VkDebugUtilsMessengerCreateInfoEXT debugCreateInfo;
    if (enableValidationLayers) {
        createInfo.enabledLayerCount = static_cast<uint32_t>(validationLayers.size());
        createInfo.ppEnabledLayerNames = validationLayers.data();

        populateDebugMessengerCreateInfo(debugCreateInfo);
        createInfo.pNext = (VkDebugUtilsMessengerCreateInfoEXT*)&debugCreateInfo;
    } else {
        createInfo.enabledLayerCount = 0;
        createInfo.pNext = nullptr;
    }

    // Attempt to actually create the instance
    // Throwing an error if it fails for any reason
    if (vkCreateInstance(&createInfo, nullptr, &instance) != VK_SUCCESS) {
        throw std::runtime_error("failed to create instance!");
    }
}

std::vector<const char*> Engine::getRequiredExtensions() {
    // Add the GLFW extensions, which are always required for this engine to work
    uint32_t glfwExtensionCount = 0;
    const char** glfwExtensions;
    glfwExtensions = glfwGetRequiredInstanceExtensions(&glfwExtensionCount);

    // Use the GLFW to make a list of all required extensions
    std::vector<const char*> extensions(glfwExtensions, glfwExtensions + glfwExtensionCount);

    // If we're in debug mode, add the validation layer extensions to our list
    if (enableValidationLayers) {
        extensions.push_back(VK_EXT_DEBUG_UTILS_EXTENSION_NAME);
    }

    return extensions;
}

bool Engine::checkValidationLayerSupport() {
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

void Engine::populateDebugMessengerCreateInfo(VkDebugUtilsMessengerCreateInfoEXT& createInfo) {
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

void Engine::setupDebugMessenger() {
    if (!enableValidationLayers) return;

    // Register our debug messenger that prints out our validation layer's logs
    VkDebugUtilsMessengerCreateInfoEXT createInfo;
    populateDebugMessengerCreateInfo(createInfo);

    if (CreateDebugUtilsMessengerEXT(instance, &createInfo, nullptr, &debugMessenger) != VK_SUCCESS) {
        throw std::runtime_error("failed to set up debug messenger!");
    }
}

void Engine::createSurface() {
    if (glfwCreateWindowSurface(instance, window, nullptr, &surface) != VK_SUCCESS) {
        throw std::runtime_error("failed to create window surface!");
    }
}

PhysicalDeviceCandidate Engine::pickPhysicalDevice() {
    // Find how many physical devices we have
    uint32_t deviceCount = 0;
    vkEnumeratePhysicalDevices(instance, &deviceCount, nullptr);

    // Throw an error if we have no physical devices
    if (deviceCount == 0) {
        throw std::runtime_error("failed to find GPUs with Vulkan support!");
    }

    // Get each physical device
    std::vector<VkPhysicalDevice> devices(deviceCount);
    vkEnumeratePhysicalDevices(instance, &deviceCount, devices.data());

    // Rate each physical device and insert each device and score into a map
    std::multimap<int, PhysicalDeviceCandidate> candidates;
    for (const auto& device : devices) {
        PhysicalDeviceCandidate candidate;
        candidate.physicalDevice = device;
        candidate.indices = findQueueFamilies(device);
        candidate.swapChainSupport = renderer.querySwapChainSupport(device);

        candidates.insert(std::make_pair(rateDeviceSuitability(candidate), candidate));
    }

    // Get the best scoring key-value pair (the map sorts on insert), and check if it has a non-zero score
    if (candidates.rbegin()->first > 0) {
        // Get the best score's device and indices (the second item in the key-value pair)
        PhysicalDeviceCandidate candidate = candidates.rbegin()->second;
        // Set the physical device
        physicalDevice = candidate.physicalDevice;
        // Return the candidate so the info can be used in other functions
        return candidate;
    } else {
        throw std::runtime_error("failed to find a suitable GPU!");
    }
}

int Engine::rateDeviceSuitability(PhysicalDeviceCandidate candidate) {
    VkPhysicalDevice device = candidate.physicalDevice;
    QueueFamilyIndices indices = candidate.indices;
    SwapChainSupportDetails swapChainSupport = candidate.swapChainSupport;

    // Find the physical properties of this device
    VkPhysicalDeviceProperties deviceProperties;
    vkGetPhysicalDeviceProperties(device, &deviceProperties);

    // Check its queue families and ensure it supports each feature we need
    // isComplete will check to make sure each feature we need exists at some index
    // We don't care what index, just that such an index exists
    // If it doesn't, return 0 to represent complete non-suitability
    if (!indices.isComplete())
        return 0;

    // Check if it has all the physical device extensions we need
    // Otherwise return 0 to represent complete non-suitability
    if (!checkDeviceExtensionSupport(device))
        return 0;

    // Check if the device supports the swap chains we need
    // And, as before, if it doesn't then return 0 to represent complete non-suitability
    if (swapChainSupport.formats.empty() || swapChainSupport.presentModes.empty())
        return 0;

    int score = 0;

    // Discrete GPUs have a significant performance advantage
    if (deviceProperties.deviceType == VK_PHYSICAL_DEVICE_TYPE_DISCRETE_GPU)
        score += 1000;

    // Maximum possible size of textures affects graphics quality
    score += deviceProperties.limits.maxImageDimension2D;

    return score;
}

bool Engine::checkDeviceExtensionSupport(VkPhysicalDevice device) {
    // Find out how many extensions the device has
    uint32_t extensionCount;
    vkEnumerateDeviceExtensionProperties(device, nullptr, &extensionCount, nullptr);

    // Get the properties for each extension
    std::vector<VkExtensionProperties> availableExtensions(extensionCount);
    vkEnumerateDeviceExtensionProperties(device, nullptr, &extensionCount, availableExtensions.data());

    // Create a set of all required extensions
    std::set<std::string> requiredExtensions(deviceExtensions.begin(), deviceExtensions.end());

    // Go through each extension the device has. Attempt to remove its name from our
    // list of required components
    for (const auto& extension : availableExtensions) {
        requiredExtensions.erase(extension.extensionName);
    }

    // If all our required extensions got removed, then this device supports all of them
    return requiredExtensions.empty();
}

QueueFamilyIndices Engine::findQueueFamilies(VkPhysicalDevice device) {
    QueueFamilyIndices indices;

    // Find out how many queue families we have
    uint32_t queueFamilyCount = 0;
    vkGetPhysicalDeviceQueueFamilyProperties(device, &queueFamilyCount, nullptr);

    // Find the properties for all our queue families
    std::vector<VkQueueFamilyProperties> queueFamilies(queueFamilyCount);
    vkGetPhysicalDeviceQueueFamilyProperties(device, &queueFamilyCount, queueFamilies.data());

    int i = 0;
    for (const auto& queueFamily : queueFamilies) {
        // Check each queue family for one that supports VK_QUEUE_GRAPHICS_BIT
        if (queueFamily.queueFlags & VK_QUEUE_GRAPHICS_BIT) {
            indices.graphicsFamily = i;
        }

        // Check each queue family for one that supports presenting to the window system
        VkBool32 presentSupport = false;
        vkGetPhysicalDeviceSurfaceSupportKHR(device, i, surface, &presentSupport);
        if (presentSupport) {
            indices.presentFamily = i;
        }

        // Break out early if every feature has an index
        if (indices.isComplete())
            break;

        i++;
    }

    return indices;
}

void Engine::createLogicalDevice(QueueFamilyIndices indices) {
    // Create information structs for each of our device queue families
    // Configure it as appropriate and give it our queue family indices
    std::vector<VkDeviceQueueCreateInfo> queueCreateInfos;
    std::set<uint32_t> uniqueQueueFamilies = { indices.graphicsFamily.value(), indices.presentFamily.value() };
    // We'll give them equal priority
    float queuePriority = 1.0f;
    for (uint32_t queueFamily : uniqueQueueFamilies) {
        VkDeviceQueueCreateInfo queueCreateInfo = {};
        queueCreateInfo.sType = VK_STRUCTURE_TYPE_DEVICE_QUEUE_CREATE_INFO;
        queueCreateInfo.queueFamilyIndex = queueFamily;
        queueCreateInfo.queueCount = 1;
        queueCreateInfo.pQueuePriorities = &queuePriority;
        queueCreateInfos.push_back(queueCreateInfo);
    }

    // List the physical device features we need our logical device to support
    VkPhysicalDeviceFeatures deviceFeatures = {};

    // Create the information struct for our logical device
    // Configuring it with our device queue and required features
    VkDeviceCreateInfo createInfo = {};
    createInfo.sType = VK_STRUCTURE_TYPE_DEVICE_CREATE_INFO;
    createInfo.queueCreateInfoCount = static_cast<uint32_t>(queueCreateInfos.size());
    createInfo.pQueueCreateInfos = queueCreateInfos.data();
    createInfo.pEnabledFeatures = &deviceFeatures;

    // Setup our device-specific extensions
    createInfo.enabledExtensionCount = static_cast<uint32_t>(deviceExtensions.size());
    createInfo.ppEnabledExtensionNames = deviceExtensions.data();

    // Setup our device-specific validation layers
    // New versions of Vulkan do not have device-specific validation layers
    // but we keep this in for backwards compatibility
    if (enableValidationLayers) {
        createInfo.enabledLayerCount = static_cast<uint32_t>(validationLayers.size());
        createInfo.ppEnabledLayerNames = validationLayers.data();
    } else {
        createInfo.enabledLayerCount = 0;
    }

    // Create the logical device and throw an error if anything goes wrong
    if (vkCreateDevice(physicalDevice, &createInfo, nullptr, &device) != VK_SUCCESS) {
        throw std::runtime_error("failed to create logical device!");
    }
}

void Engine::setupWorld(World* world) {
    this->world = world;
    world->init();
}

void Engine::mainLoop() {
    while (!glfwWindowShouldClose(window)) {
        glfwPollEvents();
        world->update();
    }
    vkDeviceWaitIdle(device);
}

void Engine::cleanup() {
    // Destroy and external objects
    if (preCleanup != nullptr) {
        preCleanup();
    }

    // Destroy our renderer
    renderer.cleanup();

    // If we're in debug mode, destroy our debug messenger
    if (enableValidationLayers) {
        DestroyDebugUtilsMessengerEXT(instance, debugMessenger, nullptr);
    }

    // Destroy our logical device
    vkDestroyDevice(device, nullptr);

    // Destroy our vulkan instance
    vkDestroySurfaceKHR(instance, surface, nullptr);
    vkDestroyInstance(instance, nullptr);

    // Destroy our GLFW window
    glfwDestroyWindow(window);

    glfwTerminate();
}
