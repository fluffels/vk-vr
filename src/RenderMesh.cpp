#pragma warning(disable: 4267)

#define STB_IMAGE_IMPLEMENTATION
#include "stb/stb_image.h"

#include "MathLib.h"
#include "RenderMesh.h"

struct Vertex {
    Vec3 position;
};

static VulkanMesh mesh;
static VulkanPipeline pipeline;
static vector<Vertex> vertices;
static vector<uint32_t> indices;

void uploadMesh(
    Vulkan& vk
) {
    initVKPipeline(
        vk,
        "default",
        pipeline
    );

    auto& v0 = vertices.emplace_back();
    v0.position = {-1, 1, 1};
    auto& v1 = vertices.emplace_back();
    v1.position = {0, -1, 1};
    auto& v2 = vertices.emplace_back();
    v2.position = {1, 1, 1};

    indices.push_back(0);
    indices.push_back(1);
    indices.push_back(2);

    uploadMesh(
        vk.device,
        vk.memories,
        vk.queueFamily,
        vertices.data(),
        vertices.size()*sizeof(Vertex),
        indices.data(),
        indices.size()*sizeof(uint32_t),
        mesh
    );

    updateUniformBuffer(
        vk.device,
        pipeline.descriptorSet,
        0,
        vk.uniforms.handle
    );
}

void renderMesh(
    VkCommandBuffer& cmd
) {
    vkCmdBindPipeline(
        cmd,
        VK_PIPELINE_BIND_POINT_GRAPHICS,
        pipeline.handle
    );
    vkCmdBindDescriptorSets(
        cmd,
        VK_PIPELINE_BIND_POINT_GRAPHICS,
        pipeline.layout,
        0,
        1,
        &pipeline.descriptorSet,
        0,
        nullptr
    );
    VkDeviceSize offsets[] = {0};

    vkCmdBindVertexBuffers(
        cmd,
        0, 1,
        &mesh.vBuff.handle,
        offsets
    );
    vkCmdBindIndexBuffer(
        cmd,
        mesh.iBuff.handle,
        0,
        VK_INDEX_TYPE_UINT32
    );
    Vec3 white = {1, 1, 1};
    vkCmdPushConstants(
        cmd,
        pipeline.layout,
        VK_SHADER_STAGE_VERTEX_BIT,
        0,
        sizeof(white),
        &white
    );
    vkCmdDrawIndexed(
        cmd,
        indices.size(),
        1, 0, 0, 0
    );
}
