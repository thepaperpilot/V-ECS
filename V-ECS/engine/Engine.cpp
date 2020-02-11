#include "Engine.h"
#include "GLFWEvents.h"
#include "../rendering/Renderer.h"
#include "../ecs/World.h"
#include "../events/EventManager.h"
#include "../input/InputEvents.h"

#include <map>
#include <set>

using namespace vecs;

void windowResizeCallback(GLFWwindow* window, int width, int height) {
    // TODO pause game if not paused already, because
    // ECS world won't update while resizing on some systems
    // except for frames where window size actually changed
    WindowResizeEvent event;
    event.width = width;
    event.height = height;
    EventManager::fire(event);
}

Engine::Engine() {
    initWindow();
    initVulkan();
}

void Engine::setupWorld(World* world) {
    this->world = world;
    world->init(device, surface, window);
}

void Engine::run() {
    mainLoop();
    cleanup();
}

void Engine::initWindow() {
    glfwInit();

    glfwWindowHint(GLFW_CLIENT_API, GLFW_NO_API);
    glfwWindowHint(GLFW_RESIZABLE, GLFW_TRUE);

    window = glfwCreateWindow(windowWidth, windowHeight, applicationName, nullptr, nullptr);

    // Enable raw mouse motion when cursor is disabled
    if (glfwRawMouseMotionSupported())
        glfwSetInputMode(window, GLFW_RAW_MOUSE_MOTION, GLFW_TRUE);

    // Register GLFW callbacks
    glfwSetWindowUserPointer(window, this);
    glfwSetWindowSizeLimits(window, 1, 1, GLFW_DONT_CARE, GLFW_DONT_CARE);
    glfwSetFramebufferSizeCallback(window, windowResizeCallback);
    glfwSetWindowRefreshCallback(window, [](GLFWwindow* window) { EventManager::fire(RefreshWindowEvent()); });
    glfwSetCursorPosCallback(window, cursorPositionCallback);
    glfwSetMouseButtonCallback(window, mouseButtonCallback);
    glfwSetKeyCallback(window, keyCallback);
}

void Engine::initVulkan() {
    createInstance();
    createSurface();

    debugger.setupDebugMessenger(instance);

    device = new Device(instance, surface);
}

void Engine::createInstance() {
    // If we're in Debug mode, ensure we have validation layer support
    // If we don't, give a warning so the developer is made aware they'll
    // need to download them (should be included in the Vulkan SDK)
    if (!debugger.checkValidationLayerSupport()) {
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
    if (debugger.enableValidationLayers) {
        createInfo.enabledLayerCount = static_cast<uint32_t>(debugger.validationLayers.size());
        createInfo.ppEnabledLayerNames = debugger.validationLayers.data();

        debugger.populateDebugMessengerCreateInfo(debugCreateInfo);
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

void Engine::createSurface() {
    if (glfwCreateWindowSurface(instance, window, nullptr, &surface) != VK_SUCCESS) {
        throw std::runtime_error("failed to create window surface!");
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
    if (debugger.enableValidationLayers) {
        extensions.push_back(VK_EXT_DEBUG_UTILS_EXTENSION_NAME);
    }

    return extensions;
}

void Engine::mainLoop() {
    lastFrameTime = glfwGetTime();
    while (!glfwWindowShouldClose(window)) {
        glfwPollEvents();

        double currentTime = glfwGetTime();
        world->update(currentTime - lastFrameTime);
        lastFrameTime = currentTime;
    }
    vkDeviceWaitIdle(*device);
}

void Engine::cleanup() {
    // Destroy and external objects
    if (preCleanup != nullptr) {
        preCleanup();
    }

    // If we're in debug mode, destroy our debug messenger
    if (debugger.enableValidationLayers) {
        debugger.cleanup(instance);
    }

    // Destroy our logical device
    device->cleanup();

    // Destroy our vulkan instance
    vkDestroySurfaceKHR(instance, surface, nullptr);
    vkDestroyInstance(instance, nullptr);

    // Destroy our GLFW window
    glfwDestroyWindow(window);

    glfwTerminate();
}
