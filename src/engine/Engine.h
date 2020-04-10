#pragma once

#include <vulkan/vulkan.h>

#include <hastyNoise/hastyNoise.h>

#include "Debugger.h"
#include "../rendering/Renderer.h"

// Forward Declarations
struct GLFWwindow;
class table;

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
        inline static size_t fastestSimd = 0;

        Device* device;
        GLFWwindow* window;
        World* world = nullptr;

        Renderer renderer;

        size_t imguiVertexBufferSize = 0;
        size_t imguiIndexBufferSize = 0;
        Buffer imguiVertexBuffer;
        Buffer imguiIndexBuffer;

        Engine() : renderer(this) {
            // init noise
            HastyNoise::loadSimd("./simd");
            fastestSimd = HastyNoise::GetFastestSIMD();
        };

        void init();

        void setWorld(World* world);

        void updateWorld();

    private:
        VkInstance instance;
        VkSurfaceKHR surface;
        World* nextWorld = nullptr;
        Debugger debugger;

        double lastFrameTime;

        void run(std::string worldFilename);

        void initWindow(sol::table manifest);
        void initVulkan(sol::table manifest);
        void initImGui();

        void createInstance(sol::table manifest);
        void createSurface();

        std::vector<const char*> getRequiredExtensions();

        void mainLoop();
        void cleanup();
    };
}
