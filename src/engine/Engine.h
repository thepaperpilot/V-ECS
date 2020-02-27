#pragma once

#include <vulkan/vulkan.h>

#include "Debugger.h"
#include "Manifest.h"
#include "../rendering/Renderer.h"

// Forward Declarations
struct GLFWwindow;

namespace vecs {

    // Forward Declarations
    class Device;
    class World;

    // A lot of this is based on the Vulkan tutorial at https://vulkan-tutorial.com
    // The Engine is what handles using Vulkan and GLFW to create a window,
    // setup our devices, and otherwise set everything up. This is the class
    // you instantiate to create your program
    // I also used this as a reference for structuring some of the vulkan things:
    // https://github.com/SaschaWillems/Vulkan
    class Engine {
    public:
        Device* device;
        GLFWwindow* window;
        World* world;

        Renderer renderer;

        Engine() : renderer(this) {};

        void init();

        void setWorld(World* world, bool init = true, bool cleanupWorld = false);

        void run();

        // This is an optional function pointer to run before the cleanup step
        // intended to be used for cleaning up anything external to the engine
        // before the logical device gets destroyed itself
        void (*preCleanup)();

    private:
        VkInstance instance;
        VkSurfaceKHR surface;
        World* nextWorld;
        Debugger debugger;
        Manifest manifest;

        double lastFrameTime;
        bool cleanupWorld;

        void initWindow();
        void initVulkan();

        void createInstance();
        void createSurface();

        std::vector<const char*> getRequiredExtensions();

        void mainLoop();
        void cleanup();
    };
}
