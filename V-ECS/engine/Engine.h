#pragma once

#include <vulkan/vulkan.h>
#include <GLFW/glfw3.h>

#include <cstdlib>
#include <functional>
#include <iostream>
#include <optional>
#include <stdexcept>

#include "../events/EventManager.h"
#include "../rendering/Renderer.h"

namespace vecs {

    // Forward Declarations
    class World;

    struct QueueFamilyIndices {
        std::optional<uint32_t> graphicsFamily;
        std::optional<uint32_t> presentFamily;

        bool isComplete() {
            // Check that every family feature has some value
            return graphicsFamily.has_value() && presentFamily.has_value();
        }
    };

    struct PhysicalDeviceCandidate {
        VkPhysicalDevice physicalDevice;
        QueueFamilyIndices indices;
        SwapChainSupportDetails swapChainSupport;
    };

    struct RefreshWindowEvent : EventData {};

    // A lot of this is based on the Vulkan tutorial at https://vulkan-tutorial.com
    // The Engine is what handles using Vulkan and GLFW to create a window,
    // setup our devices, and otherwise set everything up. This is the class
    // you instantiate to create your program
    class Engine {
        friend class Renderer;
    public:
        // How severe must an error be to be printed to stdout
        // e.g. if its set to warning, then warnings and errors get printed
        // but if its set to verbose everything will get printed
        static VkDebugUtilsMessageSeverityFlagBitsEXT logLevel;

        // These values are used for the initial size of the window,
        // but the window may be resizable (or changed via code later on)
        int windowWidth = 1280;
        int windowHeight = 720;

        // This value is used for the intial title of the window
        const char* applicationName = "A V-ECS Application";
        uint32_t applicationVersion;

        Renderer renderer;
        VkPhysicalDevice physicalDevice = VK_NULL_HANDLE;
        VkDevice device;
        GLFWwindow* window;

        void run(World* initialWorld);

        // This is an optional function pointer to run before the cleanup step
        // intended to be used for cleaning up anything external to the engine
        // before the logical device gets destroyed itself
        void (*preCleanup)();

    private:
        static VKAPI_ATTR VkBool32 VKAPI_CALL debugCallback(
            VkDebugUtilsMessageSeverityFlagBitsEXT messageSeverity,
            VkDebugUtilsMessageTypeFlagsEXT messageType,
            const VkDebugUtilsMessengerCallbackDataEXT* pCallbackData,
            void* pUserData);

        VkInstance instance;
        VkDebugUtilsMessengerEXT debugMessenger;
        VkSurfaceKHR surface;
        World* world;

        double lastFrameTime;

        void initWindow();

        void initVulkan();
        void createInstance();

        std::vector<const char*> getRequiredExtensions();

        bool checkValidationLayerSupport();
        void populateDebugMessengerCreateInfo(VkDebugUtilsMessengerCreateInfoEXT& createInfo);
        void setupDebugMessenger();

        void createSurface();

        PhysicalDeviceCandidate pickPhysicalDevice();
        int rateDeviceSuitability(PhysicalDeviceCandidate candidate);
        bool checkDeviceExtensionSupport(VkPhysicalDevice device);
        QueueFamilyIndices findQueueFamilies(VkPhysicalDevice device);

        void createLogicalDevice(QueueFamilyIndices indices);

        void setupWorld(World* world);

        void mainLoop();
        void cleanup();
    };
}
