#include "Device.h"

#include <vulkan/vulkan.h>
#include <vector>
#include <map>
#include <set>

using namespace vecs;

// List of physical device extensions that must be supported for a physical
// device to be considered at all suitable
const std::vector<const char*> deviceExtensions = {
    VK_KHR_SWAPCHAIN_EXTENSION_NAME
};

Device::Device(VkInstance instance, VkSurfaceKHR surface) {
    pickPhysicalDevice(instance, surface);
    createLogicalDevice();
    commandPool = createCommandPool(queueFamilyIndices.graphics.value());
}

VkCommandPool Device::createCommandPool(uint32_t queueFamilyIndex, VkCommandPoolCreateFlags createFlags) {
    VkCommandPoolCreateInfo cmdPoolInfo = {};
    cmdPoolInfo.sType = VK_STRUCTURE_TYPE_COMMAND_POOL_CREATE_INFO;
    cmdPoolInfo.queueFamilyIndex = queueFamilyIndex;
    cmdPoolInfo.flags = createFlags;

    VkCommandPool commandPool;
    if (vkCreateCommandPool(logical, &cmdPoolInfo, nullptr, &commandPool) != VK_SUCCESS) {
        throw std::runtime_error("failed to create logical device!");
    }

    return commandPool;
}

Buffer Device::createBuffer(VkDeviceSize size, VkBufferUsageFlags usage, VkMemoryPropertyFlags properties) {
    // Create a new struct to wrap around the VkBuffer
    Buffer buffer(&logical, size, usage, properties);

    // Create our new buffer with the given size
    VkBufferCreateInfo bufferInfo = {};
    bufferInfo.sType = VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO;
    bufferInfo.size = size;
    bufferInfo.usage = usage;
    bufferInfo.sharingMode = VK_SHARING_MODE_EXCLUSIVE;

    if (vkCreateBuffer(logical, &bufferInfo, nullptr, &buffer.buffer) != VK_SUCCESS) {
        throw std::runtime_error("failed to create vertex buffer!");
    }

    // Assign memory to our buffer
    // First define our memory requirements
    VkMemoryRequirements memRequirements;
    vkGetBufferMemoryRequirements(logical, buffer.buffer, &memRequirements);

    // Define our memory allocation request
    VkMemoryAllocateInfo allocInfo = {};
    allocInfo.sType = VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO;
    allocInfo.allocationSize = memRequirements.size;
    allocInfo.memoryTypeIndex = findMemoryType(memRequirements.memoryTypeBits, properties);

    // Allocate memory
    if (vkAllocateMemory(logical, &allocInfo, nullptr, &buffer.memory) != VK_SUCCESS) {
        throw std::runtime_error("failed to allocate vertex buffer memory!");
    }

    // Bind this memory to our new buffer
    vkBindBufferMemory(logical, buffer.buffer, buffer.memory, 0);

    return buffer;
}

void Device::copyBuffer(Buffer* src, Buffer* dest, VkQueue queue, VkBufferCopy* copyRegion) {
    // Get a command buffer to use
    VkCommandBuffer copyCmd = createCommandBuffer(VK_COMMAND_BUFFER_LEVEL_PRIMARY);

    // Determine how much to copy
    VkBufferCopy bufferCopy;
    if (copyRegion == nullptr)
        bufferCopy.size = src->size;
    else
        bufferCopy = *copyRegion;

    // Copy the buffers
    vkCmdCopyBuffer(copyCmd, src->buffer, dest->buffer, 1, &bufferCopy);

    // End our command buffer and submit it
    submitCommandBuffer(copyCmd, queue);
}

VkCommandBuffer Device::createCommandBuffer(VkCommandBufferLevel level, VkCommandPool commandPool, bool begin) {
    if (commandPool == nullptr)
        commandPool = this->commandPool;

    // Tell it which pool to create a command buffer for, with the given level
    VkCommandBufferAllocateInfo allocInfo = {};
    allocInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO;
    allocInfo.commandPool = commandPool;
    allocInfo.level = level;
    allocInfo.commandBufferCount = 1;

    // Create the buffer
    VkCommandBuffer buffer;
    if (vkAllocateCommandBuffers(logical, &allocInfo, &buffer) != VK_SUCCESS) {
        throw std::runtime_error("failed to allocate command buffer!");
    }

    if (begin) beginCommandBuffer(buffer);

    return buffer;
}

std::vector<VkCommandBuffer> Device::createCommandBuffers(VkCommandBufferLevel level, int amount, VkCommandPool commandPool, bool begin) {
    if (commandPool == nullptr)
        commandPool = this->commandPool;

    // Tell it which pool to create command buffers for, and how many, with the given level
    VkCommandBufferAllocateInfo allocInfo = {};
    allocInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO;
    allocInfo.commandPool = commandPool;
    allocInfo.level = level;
    allocInfo.commandBufferCount = amount;

    // Create the buffers
    std::vector<VkCommandBuffer> buffers(amount);
    if (vkAllocateCommandBuffers(logical, &allocInfo, buffers.data()) != VK_SUCCESS) {
        throw std::runtime_error("failed to allocate command buffer!");
    }

    if (begin)
        for (VkCommandBuffer buffer : buffers) beginCommandBuffer(buffer);

    return buffers;
}

void Device::submitCommandBuffer(VkCommandBuffer buffer, VkQueue queue, VkCommandPool commandPool, bool free) {
    if (commandPool == nullptr)
        commandPool = this->commandPool;

    if (vkEndCommandBuffer(buffer) != VK_SUCCESS) {
        throw std::runtime_error("failed to record command buffer!");
    }

    // Submit our command buffer to its queue
    VkSubmitInfo submitInfo = {};
    submitInfo.sType = VK_STRUCTURE_TYPE_SUBMIT_INFO;
    submitInfo.commandBufferCount = 1;
    submitInfo.pCommandBuffers = &buffer;

    vkQueueSubmit(queue, 1, &submitInfo, VK_NULL_HANDLE);
    vkQueueWaitIdle(queue);

    // Destroy our command buffer
    if (free) vkFreeCommandBuffers(logical, commandPool, 1, &buffer);
}

uint32_t Device::findMemoryType(uint32_t typeFilter, VkMemoryPropertyFlags properties) {
    // Get our different types of memories on our physical device
    VkPhysicalDeviceMemoryProperties memProperties;
    vkGetPhysicalDeviceMemoryProperties(physical, &memProperties);

    for (uint32_t i = 0; i < memProperties.memoryTypeCount; i++) {
        // Check if this type of memory supports our filter and has all the properties we need
        // TODO rank and choose best memory type (e.g. use VRAM before swap)
        if ((typeFilter & (1 << i)) && (memProperties.memoryTypes[i].propertyFlags & properties) == properties) {
            return i;
        }
    }

    // Throw error if we can't find any memory that suits our needs
    throw std::runtime_error("failed to find suitable memory type!");
}

void Device::cleanup() {
    // Destroy our command pool
    vkDestroyCommandPool(logical, commandPool, nullptr);

    // Destroy our logical device
    vkDestroyDevice(logical, nullptr);
}

void Device::pickPhysicalDevice(VkInstance instance, VkSurfaceKHR surface) {
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
        candidate.indices = findQueueFamilies(device, surface);
        candidate.swapChainSupport = querySwapChainSupport(device, surface);

        candidates.insert(std::make_pair(rateDeviceSuitability(candidate), candidate));
    }

    // Get the best scoring key-value pair (the map sorts on insert), and check if it has a non-zero score
    if (candidates.rbegin()->first > 0) {
        // Get the best score's device and indices (the second item in the key-value pair)
        PhysicalDeviceCandidate candidate = candidates.rbegin()->second;
        // Set the physical device
        physical = candidate.physicalDevice;
        queueFamilyIndices = candidate.indices;
        swapChainSupport = candidate.swapChainSupport;
    } else {
        throw std::runtime_error("failed to find a suitable GPU!");
    }
}

QueueFamilyIndices Device::findQueueFamilies(VkPhysicalDevice device, VkSurfaceKHR surface) {
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
            indices.graphics = i;
        }

        // Check each queue family for one that supports presenting to the window system
        VkBool32 presentSupport = false;
        vkGetPhysicalDeviceSurfaceSupportKHR(device, i, surface, &presentSupport);
        if (presentSupport) {
            indices.present = i;
        }

        // Break out early if every feature has an index
        if (indices.isComplete())
            break;

        i++;
    }

    return indices;
}

SwapChainSupportDetails Device::querySwapChainSupport(VkPhysicalDevice physicalDevice, VkSurfaceKHR surface) {
    SwapChainSupportDetails details;

    // Get the capabilities of our surface
    vkGetPhysicalDeviceSurfaceCapabilitiesKHR(physicalDevice, surface, &details.capabilities);

    // Find how many surface formats the physical device supports
    uint32_t formatCount;
    vkGetPhysicalDeviceSurfaceFormatsKHR(physicalDevice, surface, &formatCount, nullptr);
    // If the number of formats supported is non-zero, add them to our details.formats list
    if (formatCount != 0) {
        details.formats.resize(formatCount);
        vkGetPhysicalDeviceSurfaceFormatsKHR(physicalDevice, surface, &formatCount, details.formats.data());
    }

    // Find how many present modes the physical device supports
    uint32_t presentModeCount;
    vkGetPhysicalDeviceSurfacePresentModesKHR(physicalDevice, surface, &presentModeCount, nullptr);
    // If the number of modes is non-zero, add them to our details.presentModes list
    if (presentModeCount != 0) {
        details.presentModes.resize(presentModeCount);
        vkGetPhysicalDeviceSurfacePresentModesKHR(physicalDevice, surface, &presentModeCount, details.presentModes.data());
    }

    return details;
}

int Device::rateDeviceSuitability(PhysicalDeviceCandidate candidate) {
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

    // Find the physical features of the device
    VkPhysicalDeviceFeatures supportedFeatures;
    vkGetPhysicalDeviceFeatures(device, &supportedFeatures);

    // If the device doesn't support anisotropy, its non-suitable
    if (!supportedFeatures.samplerAnisotropy)
        return 0;

    int score = 0;

    // Discrete GPUs have a significant performance advantage
    if (deviceProperties.deviceType == VK_PHYSICAL_DEVICE_TYPE_DISCRETE_GPU)
        score += 1000;

    // Maximum possible size of textures affects graphics quality
    score += deviceProperties.limits.maxImageDimension2D;

    return score;
}

bool Device::checkDeviceExtensionSupport(VkPhysicalDevice device) {
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

void Device::createLogicalDevice() {
    // Create information structs for each of our device queue families
    // Configure it as appropriate and give it our queue family indices
    std::vector<VkDeviceQueueCreateInfo> queueCreateInfos;
    std::set<uint32_t> uniqueQueueFamilies = { queueFamilyIndices.graphics.value(), queueFamilyIndices.present.value() };
    
    // We'll give them equal priority
    const float queuePriority = 1.0f;
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
    deviceFeatures.samplerAnisotropy = VK_TRUE;

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
    
    // Create the logical device and throw an error if anything goes wrong
    if (vkCreateDevice(physical, &createInfo, nullptr, &logical) != VK_SUCCESS) {
        throw std::runtime_error("failed to create logical device!");
    }
}

void Device::beginCommandBuffer(VkCommandBuffer buffer) {
    VkCommandBufferBeginInfo beginInfo = {};
    beginInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO;

    // Being the command buffer
    if (vkBeginCommandBuffer(buffer, &beginInfo) != VK_SUCCESS) {
        throw std::runtime_error("failed to begin recording command buffer!");
    }
}
