#include "SecondaryCommandBuffer.h"

using namespace vecs;

void SecondaryCommandBuffer::updateInheritanceInfo() {
	inheritanceInfo = {};
	inheritanceInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_INHERITANCE_INFO;
	inheritanceInfo.framebuffer = renderer->swapChainFramebuffers[frameOffset];
	inheritanceInfo.renderPass = renderer->renderPass;
	inheritanceInfo.subpass = 0;
}
