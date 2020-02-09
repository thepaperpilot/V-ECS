#pragma once

#include <vulkan/vulkan.h>

#include "Debugger.h"

// Forward Declarations
struct GLFWwindow;

namespace vecs {

    // Forward Declarations
    class Renderer;
    class Device;
    class World;

    // A lot of this is based on the Vulkan tutorial at https://vulkan-tutorial.com
    // The Engine is what handles using Vulkan and GLFW to create a window,
    // setup our devices, and otherwise set everything up. This is the class
    // you instantiate to create your program
    // I also used this as a reference for structuring some of the vulkan things:
    // https://github.com/SaschaWillems/Vulkan
    class Engine {
        friend class Renderer;
    public:
        // These values are used for the initial size of the window,
        // but the window may be resizable (or changed via code later on)
        int windowWidth = 1280;
        int windowHeight = 720;

        // This value is used for the intial title of the window
        const char* applicationName = "A V-ECS Application";
        uint32_t applicationVersion;

        Renderer* renderer;
        Device* device;
        GLFWwindow* window;

        void run(World* initialWorld);

        // This is an optional function pointer to run before the cleanup step
        // intended to be used for cleaning up anything external to the engine
        // before the logical device gets destroyed itself
        void (*preCleanup)();

    private:
        VkInstance instance;
        VkSurfaceKHR surface;
        World* world;
        Debugger debugger;

        double lastFrameTime;

        void initWindow();
        void initVulkan();

        void createInstance();
        void createSurface();
        void setupWorld(World* world);

        std::vector<const char*> getRequiredExtensions();

        void mainLoop();
        void cleanup();
    };
}
