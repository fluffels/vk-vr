#pragma once

#include "MathLib.h"
#include "Uniforms.h"
#include "Vulkan.h"

struct Render {
    VkFramebuffer offscreenFramebuffer;
    VulkanSampler offscreenSampler;
    vector<VkCommandBuffer> meshCmds;
    vector<VkCommandBuffer> postProcessCmds;
    vector<VkCommandBuffer> textCmds;
};

void renderInit(Vulkan& vk, Render& render, Uniforms& uniforms);
void renderFrame(Vulkan& vk, Render& render, char* debugString);
