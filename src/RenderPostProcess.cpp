#pragma warning(disable: 4267)

#include "MathLib.h"
#include "RenderPostProcess.h"

void renderPostProcess(
    Vulkan& vk,
    VkCommandBuffer cmd
) {
    VulkanPipeline pipeline;
    initVKPipeline(
        vk,
        "postprocess",
        pipeline
    );

    const size_t componentCount = 4;
    const size_t vertexCount = 4;
    float vertices[vertexCount * componentCount] = {
        -1, 1, 0, 1,
        -1, -1, 0, 1,
        1, -1, 0, 1,
        1, 1, 0, 1
    };

    const size_t indexCount = 6;
    uint32_t indices[indexCount] = {
        0, 1, 2, 2, 3, 0
    };

    VulkanMesh mesh;
    uploadMesh(
        vk.device,
        vk.memories,
        vk.queueFamily,
        vertices,
        vertexCount*componentCount*sizeof(float),
        indices,
        indexCount*sizeof(uint32_t),
        mesh
    );

    vkCmdBindPipeline(
        cmd,
        VK_PIPELINE_BIND_POINT_GRAPHICS,
        pipeline.handle
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
    vkCmdDrawIndexed(
        cmd,
        indexCount,
        1, 0, 0, 0
    );
}
