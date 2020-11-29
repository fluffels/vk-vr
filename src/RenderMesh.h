#include <vector>

#include "Vulkan.h"

using namespace std;

void uploadMesh(
    Vulkan& vk
);

void recordMesh(
    Vulkan&,
    VkFramebuffer,
    VkCommandBuffer&
);
