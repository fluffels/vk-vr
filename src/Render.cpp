#pragma warning(disable: 4018)

#include <openvr.h>

#include "Render.h"
#include "RenderMesh.h"
#include "RenderPostProcess.h"
#include "RenderText.h"
#include "State.h"

#include "MathLib.cpp"

void renderInit(Vulkan& vk, Render& render, Uniforms& uniforms) {
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

    VulkanImage offscreenDepth = {};
    createVulkanDepthBuffer(
        vk.device,
        vk.memories,
        vk.swap.extent,
        vk.queueFamily,
        offscreenDepth
    );

    createPrepassImage(
        vk.device,
        vk.memories,
        vk.swap.extent,
        vk.queueFamily,
        vk.swap.format,
        render.offscreenSampler
    );

    VkImageView imageViews[] = {
        render.offscreenSampler.image.view,
        offscreenDepth.view
    };
    VkFramebufferCreateInfo createInfo = {};
    createInfo.sType = VK_STRUCTURE_TYPE_FRAMEBUFFER_CREATE_INFO;
    createInfo.attachmentCount = 2;
    createInfo.pAttachments = imageViews;
    createInfo.renderPass = offscreenPass;
    createInfo.height = vk.swap.extent.height;
    createInfo.width = vk.swap.extent.width;
    createInfo.layers = 1;
    checkSuccess(
        vkCreateFramebuffer(
            vk.device,
            &createInfo,
            nullptr,
            &render.offscreenFramebuffer
        )
    );

    uploadMesh(vk);
    auto framebufferCount = (uint32_t)vk.swap.images.size();
    createCommandBuffers(
        vk.device,
        vk.cmdPool,
        framebufferCount,
        render.meshCmds
    );
    for (int i = 0; i < framebufferCount; i++) {
        auto& cmd = render.meshCmds[i];

        VkClearValue colorClear;
        colorClear.color = {};
        VkClearValue depthClear;
        depthClear.depthStencil = { 1.f, 0 };
        VkClearValue clears[] = { colorClear, depthClear };

        VkRenderPassBeginInfo beginInfo = {};
        beginInfo.sType = VK_STRUCTURE_TYPE_RENDER_PASS_BEGIN_INFO;
        beginInfo.clearValueCount = 2;
        beginInfo.pClearValues = clears;
        beginInfo.framebuffer = render.offscreenFramebuffer;
        beginInfo.renderArea.extent = vk.swap.extent;
        beginInfo.renderArea.offset = {0, 0};
        beginInfo.renderPass = offscreenPass;

        beginFrameCommandBuffer(cmd);
        vkCmdBeginRenderPass(cmd, &beginInfo, VK_SUBPASS_CONTENTS_INLINE);
            renderMesh(cmd);
        vkCmdEndRenderPass(cmd);
        checkSuccess(vkEndCommandBuffer(cmd));
    }

    createCommandBuffers(
        vk.device,
        vk.cmdPool,
        framebufferCount,
        render.postProcessCmds
    );
    for (int i = 0; i < framebufferCount; i++) {
        auto& cmd = render.postProcessCmds[i];
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
            renderPostProcess(vk, render.offscreenSampler, cmd);
        vkCmdEndRenderPass(cmd);
        checkSuccess(vkEndCommandBuffer(cmd));
    }
}

void renderFrame(Vulkan& vk, Render& render, char* debugString) {
    vr::TrackedDevicePose_t poses[ vr::k_unMaxTrackedDeviceCount ];
    vr::VRCompositor()->WaitGetPoses(poses, vr::k_unMaxTrackedDeviceCount, NULL, 0 );

    recordTextCommandBuffers(vk, render.textCmds, debugString);
    vector<vector<VkCommandBuffer>> cmdss;
    cmdss.push_back(render.meshCmds);
    cmdss.push_back(render.postProcessCmds);
    cmdss.push_back(render.textCmds);
    present(vk, cmdss);

    vr::VRTextureBounds_t bounds;
    bounds.uMin = 0.0f;
    bounds.uMax = 1.0f;
    bounds.vMin = 0.0f;
    bounds.vMax = 1.0f;

    vr::VRVulkanTextureData_t vulkanData;
    vulkanData.m_nImage = (uint64_t) render.offscreenSampler.image.handle;
    // TODO(jan): what are these types and are they strictly speaking compatible?
    vulkanData.m_pDevice = (VkDevice_T*) &vk.device;
    vulkanData.m_pPhysicalDevice = (VkPhysicalDevice_T*) &vk.gpu;
    vulkanData.m_pInstance = (VkInstance_T*) &vk.handle;
    vulkanData.m_pQueue = (VkQueue_T*) &vk.queue;
    vulkanData.m_nQueueFamilyIndex = vk.queueFamily;

    // TODO(jan): we should probably be rendering at the oculus's native resolution here
    vulkanData.m_nWidth = vk.swap.extent.width;
    vulkanData.m_nHeight = vk.swap.extent.height;
    vulkanData.m_nFormat = VK_FORMAT_R8G8B8A8_SRGB;
    // TODO(jan): base this on configured sample count
    vulkanData.m_nSampleCount = 1;

    vr::Texture_t texture = { &vulkanData, vr::TextureType_Vulkan, vr::ColorSpace_Auto };
    vr::VRCompositor()->Submit( vr::Eye_Left, &texture, &bounds );
    
    // TODO(jan): render right eye
    // vulkanData.m_nImage = ( uint64_t ) m_rightEyeDesc.m_pImage;
    vr::VRCompositor()->Submit( vr::Eye_Right, &texture, &bounds );

    resetTextCommandBuffers(vk, render.textCmds);
}
