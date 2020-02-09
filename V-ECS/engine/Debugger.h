#pragma once

#include <vulkan/vulkan.h>
#include <vector>

namespace vecs {

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

		void setupDebugMessenger(VkInstance instance);
		bool checkValidationLayerSupport();
		void populateDebugMessengerCreateInfo(VkDebugUtilsMessengerCreateInfoEXT& createInfo);
		void cleanup(VkInstance instance);

	private:
		static VKAPI_ATTR VkBool32 VKAPI_CALL debugCallback(
			VkDebugUtilsMessageSeverityFlagBitsEXT messageSeverity,
			VkDebugUtilsMessageTypeFlagsEXT messageType,
			const VkDebugUtilsMessengerCallbackDataEXT* pCallbackData,
			void* pUserData);

		VkDebugUtilsMessengerEXT debugMessenger;
	};
}
