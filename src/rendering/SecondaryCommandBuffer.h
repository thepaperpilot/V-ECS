#pragma once

#include "Renderer.h"

#include <vulkan\vulkan.h>

namespace vecs {
	
	class SecondaryCommandBuffer {
	public:
		VkCommandBuffer value;
		VkCommandBufferInheritanceInfo inheritanceInfo;

		uint32_t frameOffset;

		operator VkCommandBuffer() { return value; }

		SecondaryCommandBuffer(Renderer* renderer, VkCommandBuffer value, uint32_t frameOffset) : renderer(renderer), value(value), frameOffset(frameOffset) {
			updateInheritanceInfo();
		}

		void updateInheritanceInfo();

	private:
		Renderer* renderer;
	};
}
