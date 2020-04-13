#pragma once

#include <vulkan/vulkan.h>

#include <string>
#include <vector>

namespace vecs {

	typedef enum DebugLevel {
		DEBUG_LEVEL_ERROR,
		DEBUG_LEVEL_WARN,
		DEBUG_LEVEL_INFO,
		DEBUG_LEVEL_VERBOSE
	} DebugLevel;

	struct Log {
	public:
		Log(DebugLevel level, std::string message) {
			this->level = level;
			this->message = message;
		}

		DebugLevel level;
		std::string message;
	};

	class Debugger {
	public:
		// List of validation layers to use in debug mode
		// These will check our code to make sure it doesn't get into an invalid state
		const std::vector<const char*> validationLayers = {
			"VK_LAYER_KHRONOS_validation"
		};

#ifdef NDEBUG
		const bool enableValidationLayers = false;
#else
		const bool enableValidationLayers = true;
#endif

		// How severe must an error be to be printed to stdout
		// e.g. if its set to warning, then warnings and errors get printed
		// but if its set to verbose everything will get printed
		static VkDebugUtilsMessageSeverityFlagBitsEXT logLevel;

		static void addLog(DebugLevel debugLevel, std::string message);
		static std::vector<Log> getLog() { return log; };
		static void clearLog() { log.clear(); };

		void setupDebugMessenger(VkInstance instance);
		bool checkValidationLayerSupport();
		void populateDebugMessengerCreateInfo(VkDebugUtilsMessengerCreateInfoEXT& createInfo);
		void cleanup(VkInstance instance);

	private:
		static std::vector<Log> log;

		static VKAPI_ATTR VkBool32 VKAPI_CALL debugCallback(
			VkDebugUtilsMessageSeverityFlagBitsEXT messageSeverity,
			VkDebugUtilsMessageTypeFlagsEXT messageType,
			const VkDebugUtilsMessengerCallbackDataEXT* pCallbackData,
			void* pUserData);

		VkDebugUtilsMessengerEXT debugMessenger;
	};
}
