#pragma once

#include <vulkan/vulkan.h>

#include <hastyNoise/hastyNoise.h>
#include <filesystem>
namespace fs = std::filesystem;

#define SOL_DEFAULT_PASS_ON_ERROR 1
#define SOL_ALL_SAFETIES_ON 1
#include <sol\sol.hpp>

#include "Debugger.h"
#include "Buffer.h"
#include "../rendering/Renderer.h"
#include "../jobs/JobManager.h"

// Forward Declarations
struct GLFWwindow;
class table;

namespace vecs {

    // Forward Declarations
    class Device;
    class World;
    class WorldLoadStatus;

    // A lot of this is based on the Vulkan tutorial at https://vulkan-tutorial.com
    // The Engine is what handles using Vulkan and GLFW to create a window,
    // setup our devices, and otherwise set everything up. This is the class
    // you instantiate to create your program
    // I also used this as a reference for structuring some of the vulkan things:
    // https://github.com/SaschaWillems/Vulkan
    class Engine {
    public:
        size_t fastestSimd = 0;

        Device* device;
        GLFWwindow* window;
        World* world = nullptr;
        World* nextWorld = nullptr;
        int nextInputMode = -1;

        Renderer renderer;
        Debugger debugger;
        JobManager jobManager;

        size_t imguiVertexBufferSize = 0;
        size_t imguiIndexBufferSize = 0;
        Buffer imguiVertexBuffer;
        Buffer imguiIndexBuffer;

        // We want to make sure we're using different graphics queues for different threads
        // At the moment the first queue goes to the Renderer, and worlds will use either the second or third,
        // flip-flopping so that the active world and loading world will use different queues
        // Once we have a full job system the worker threads will persist across worlds and we won't need to get
        // a queue every time we load a world
        bool nextQueueIndex = false;

        Engine() : renderer(this), jobManager(this) {
            // init noise
            HastyNoise::loadSimd("./simd");
            fastestSimd = HastyNoise::GetFastestSIMD();
        };

        void init();

        WorldLoadStatus* setWorld(std::string filename);

        void updateWorld();

    private:
        VkInstance instance;
        VkSurfaceKHR surface;

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
