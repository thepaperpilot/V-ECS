#include "Engine.h"
#include "../rendering/Renderer.h"
#include "../ecs/World.h"
#include "../events/EventManager.h"
#include "../events/GLFWEvents.h"

#include <GLFW/glfw3.h>

#include <imgui.h>
#include <examples\imgui_impl_glfw.h>

#define SOL_DEFAULT_PASS_ON_ERROR 1
#define SOL_ALL_SAFETIES_ON 1
#include <sol/sol.hpp>

#include <map>
#include <set>

using namespace vecs;

// Moved from constructor to separate function to avoid a
// static initialization order fiasco with EventManager
void Engine::init() {
    sol::state lua;
    lua.open_libraries(sol::lib::base);

    // Load application manifest
    sol::table manifest = lua.script_file("resources/manifest.lua");

    initWindow(manifest);
    initVulkan(manifest);
    initImGui();
    renderer.init(device, surface, window);

    run(manifest["initialWorld"]);
}

void Engine::setWorld(std::string filename) {
    if (this->world == nullptr) {
        this->world = new World(this, filename);
    } else {
        this->nextWorld = filename;
    }
}

void Engine::run(std::string worldFilename) {
    // Load initial world
    setWorld("resources/" + worldFilename);

    mainLoop();
    cleanup();
}

void Engine::initWindow(sol::table manifest) {
    glfwInit();

    glfwWindowHint(GLFW_CLIENT_API, GLFW_NO_API);
    glfwWindowHint(GLFW_RESIZABLE, GLFW_TRUE);

    std::string windowTitle = manifest["name"];
    window = glfwCreateWindow(manifest["width"], manifest["height"], windowTitle.c_str(), nullptr, nullptr);

    // Hunter version is out of date, so this feature has been disabled:
    // Enable raw mouse motion when cursor is disabled
    //if (glfwRawMouseMotionSupported())
    //    glfwSetInputMode(window, GLFW_RAW_MOUSE_MOTION, GLFW_TRUE);

    // Register GLFW callbacks
    glfwSetWindowUserPointer(window, this);
    glfwSetWindowSizeLimits(window, 1, 1, GLFW_DONT_CARE, GLFW_DONT_CARE);
    glfwSetFramebufferSizeCallback(window, windowResizeCallback);
    glfwSetWindowRefreshCallback(window, windowRefreshCallback);
    glfwSetCursorPosCallback(window, cursorPositionCallback);
    glfwSetMouseButtonCallback(window, mouseButtonCallback);
    glfwSetKeyCallback(window, keyCallback);
    glfwSetScrollCallback(window, scrollCallback);
}

void Engine::initVulkan(sol::table manifest) {
    createInstance(manifest);
    createSurface();

    debugger.setupDebugMessenger(instance);

    device = new Device(instance, surface);
}

void Engine::initImGui() {
    // Setup Dear ImGui context
    IMGUI_CHECKVERSION();
    ImGui::CreateContext();

    // Setup Dear ImGui style
    ImGui::StyleColorsDark();
    //ImGui::StyleColorsClassic();

    // Setup GLFW + Vulkan implementation
    ImGui_ImplGlfw_InitForVulkan(window, true);

    // Setup default font first, so it gets used as the default :)
    ImGui::GetIO().Fonts->AddFontDefault();
}

void Engine::createInstance(sol::table manifest) {
    // If we're in Debug mode, ensure we have validation layer support
    // If we don't, give a warning so the developer is made aware they'll
    // need to download them (should be included in the Vulkan SDK)
    if (!debugger.checkValidationLayerSupport()) {
        throw std::runtime_error("validation layers requested, but not available!");
    }

    // Create our application info. Confiures the values both hard-coded by the engine
    // as well as some loaded in from the manifest
    std::string windowTitle = manifest["name"];
    VkApplicationInfo appInfo = {};
    appInfo.sType = VK_STRUCTURE_TYPE_APPLICATION_INFO;
    appInfo.pApplicationName = windowTitle.c_str();
    appInfo.applicationVersion = VK_MAKE_VERSION(1, 0, 0);
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
    VK_CHECK_RESULT(vkCreateInstance(&createInfo, nullptr, &instance));
}

void Engine::createSurface() {
    VK_CHECK_RESULT(glfwCreateWindowSurface(instance, window, nullptr, &surface));
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
        // TODO find out how to update world efficiently when glfwPollEvents blocks the main thread
        glfwPollEvents();
        updateWorld();
    }
    vkDeviceWaitIdle(*device);
}

void Engine::updateWorld() {
    double currentTime = glfwGetTime();
    vkDeviceWaitIdle(*device);
    renderer.acquireImage();
    vkDeviceWaitIdle(*device);
    world->update(currentTime - lastFrameTime);
    vkDeviceWaitIdle(*device);
    renderer.presentImage();
    lastFrameTime = currentTime;

    if (nextWorld != "") {
        World* world = new World(this, nextWorld);
        if (world->isValid) {
            vkDeviceWaitIdle(*device);
            // Handle world trade-off
            this->world->cleanup();
            this->world = world;
            // TODO find way to make windowRefresh faster in this situation
            // Also may not be necessary anymore?
            this->world->windowRefresh(true, renderer.imageCount);
        }
        nextWorld = "";
    }
}

void Engine::cleanup() {
    // Cleanup our active world
    if (world != nullptr)
        world->cleanup();

    // If we're in debug mode, destroy our debug messenger
    if (debugger.enableValidationLayers) {
        debugger.cleanup(instance);
    }

    ImGui_ImplGlfw_Shutdown();
    if (imguiVertexBuffer.buffer != VK_NULL_HANDLE)
        device->cleanupBuffer(imguiVertexBuffer);
    if (imguiIndexBuffer.buffer != VK_NULL_HANDLE)
        device->cleanupBuffer(imguiIndexBuffer);

    renderer.cleanup();

    // Destroy our logical device
    device->cleanup();

    // Destroy our vulkan instance
    vkDestroySurfaceKHR(instance, surface, nullptr);
    vkDestroyInstance(instance, nullptr);

    // Destroy our GLFW window
    glfwDestroyWindow(window);
    window = nullptr;

    glfwTerminate();
}
