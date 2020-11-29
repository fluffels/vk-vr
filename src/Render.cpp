#include "Render.h"
#include "RenderMesh.h"
#include "RenderText.h"
#include "State.h"

#include "MathLib.cpp"

vector<VkCommandBuffer> meshCmds;
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

    uploadMesh(vk);
    auto framebufferCount = (uint32_t)vk.swap.images.size();
    createCommandBuffers(vk.device, vk.cmdPool, framebufferCount, meshCmds);
    for (int i = 0; i < framebufferCount; i++) {
        auto& cmds = meshCmds[i];
        auto& framebuffer = vk.swap.framebuffers[i];
        recordMesh(vk, framebuffer, cmds);
    }
}

void renderFrame(Vulkan& vk, char* debugString) {
    recordTextCommandBuffers(vk, textCmds, debugString);
    vector<vector<VkCommandBuffer>> cmdss;
    cmdss.push_back(meshCmds);
    cmdss.push_back(textCmds);
    present(vk, cmdss);
    resetTextCommandBuffers(vk, textCmds);
}
