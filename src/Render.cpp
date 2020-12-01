#pragma warning(disable: 4018)

#include "Render.h"
#include "RenderMesh.h"
#include "RenderPostProcess.h"
#include "RenderText.h"
#include "State.h"

#include "MathLib.cpp"

vector<VkCommandBuffer> meshCmds;
vector<VkCommandBuffer> postProcessCmds;
vector<VkCommandBuffer> textCmds;

void renderInit(Vulkan& vk, Uniforms& uniforms) {
    auto fov = toRadians(45);
    auto height = vk.swap.extent.height;
    auto width = vk.swap.extent.width;
    auto nearz = .1f;
    auto farz = 10.f;
    matrixProjection(width, height, fov, farz, nearz, uniforms.proj);

    eventPositionReset(uniforms);

    quaternionInit(uniforms.rotation);

    VkRenderPass offscreenPass = {};
    createRenderPass(vk, true, true, offscreenPass);

    VulkanSampler offscreenSampler = {};
    createPrepassImage(
        vk.device,
        vk.memories,
        vk.swap.extent,
        vk.queueFamily,
        vk.swap.format,
        offscreenSampler
    );

    // TODO(jan): don't reuse depth buffer
    VkImageView imageViews[] = { offscreenSampler.image.view, vk.depth.view };
    VkFramebufferCreateInfo createInfo = {};
    createInfo.sType = VK_STRUCTURE_TYPE_FRAMEBUFFER_CREATE_INFO;
    createInfo.attachmentCount = 2;
    createInfo.pAttachments = imageViews;
    createInfo.renderPass = offscreenPass;
    createInfo.height = vk.swap.extent.height;
    createInfo.width = vk.swap.extent.width;
    createInfo.layers = 1;
    VkFramebuffer offscreenFramebuffer = {};
    checkSuccess(vkCreateFramebuffer(vk.device, &createInfo, nullptr, &offscreenFramebuffer));

    uploadMesh(vk);
    auto framebufferCount = (uint32_t)vk.swap.images.size();
    createCommandBuffers(vk.device, vk.cmdPool, framebufferCount, meshCmds);
    for (int i = 0; i < framebufferCount; i++) {
        auto& cmd = meshCmds[i];

        VkClearValue colorClear;
        colorClear.color = {};
        VkClearValue depthClear;
        depthClear.depthStencil = { 1.f, 0 };
        VkClearValue clears[] = { colorClear, depthClear };

        VkRenderPassBeginInfo beginInfo = {};
        beginInfo.sType = VK_STRUCTURE_TYPE_RENDER_PASS_BEGIN_INFO;
        beginInfo.clearValueCount = 2;
        beginInfo.pClearValues = clears;
        beginInfo.framebuffer = offscreenFramebuffer;
        beginInfo.renderArea.extent = vk.swap.extent;
        beginInfo.renderArea.offset = {0, 0};
        beginInfo.renderPass = offscreenPass;

        beginFrameCommandBuffer(cmd);
        vkCmdBeginRenderPass(cmd, &beginInfo, VK_SUBPASS_CONTENTS_INLINE);
            renderMesh(cmd);
        vkCmdEndRenderPass(cmd);
        checkSuccess(vkEndCommandBuffer(cmd));
    }

    createCommandBuffers(vk.device, vk.cmdPool, framebufferCount, postProcessCmds);
    for (int i = 0; i < framebufferCount; i++) {
        auto& cmd = postProcessCmds[i];
        auto& framebuffer = vk.swap.framebuffers[i];

        VkClearValue colorClear;
        colorClear.color = {};
        VkClearValue depthClear;
        depthClear.depthStencil = { 1.f, 0 };
        VkClearValue clears[] = { colorClear, depthClear };

        VkRenderPassBeginInfo beginInfo = {};
        beginInfo.sType = VK_STRUCTURE_TYPE_RENDER_PASS_BEGIN_INFO;
        beginInfo.clearValueCount = 2;
        beginInfo.pClearValues = clears;
        beginInfo.framebuffer = framebuffer;
        beginInfo.renderArea.extent = vk.swap.extent;
        beginInfo.renderArea.offset = {0, 0};
        beginInfo.renderPass = vk.renderPass;

        beginFrameCommandBuffer(cmd);
        vkCmdBeginRenderPass(cmd, &beginInfo, VK_SUBPASS_CONTENTS_INLINE);
            renderPostProcess(vk, offscreenSampler, cmd);
        vkCmdEndRenderPass(cmd);
        checkSuccess(vkEndCommandBuffer(cmd));
    }
}

void renderFrame(Vulkan& vk, char* debugString) {
    recordTextCommandBuffers(vk, textCmds, debugString);
    vector<vector<VkCommandBuffer>> cmdss;
    cmdss.push_back(meshCmds);
    cmdss.push_back(postProcessCmds);
    cmdss.push_back(textCmds);
    present(vk, cmdss);
    resetTextCommandBuffers(vk, textCmds);
}
