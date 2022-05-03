#define WIN32_LEAN_AND_MEAN
#define VK_USE_PLATFORM_WIN32_KHR
#include <vulkan/vulkan.h>

#define INCLUDE_SRC
#include "steamworks-c/steam_wrapper.h"

#include <stdio.h>
#include <stdint.h>
#include <stdarg.h>

#define persist static
#define global static
#define internal static
#define true 1
#define false 0
typedef uint8_t u8;
typedef int8_t i8;
typedef uint16_t u16;
typedef int16_t i16;
typedef uint32_t b32;
typedef uint32_t u32;
typedef int32_t i32;
typedef uint64_t u64;
typedef int64_t i64;
typedef uintptr_t usize;
typedef intptr_t isize;
typedef struct V2 {
    float x, y;
} V2;
typedef struct V3 {
    float x, y, z;
} V3;

static long read_file(const char *filename, u8 **output)
{
    FILE *f = fopen(filename, "rb");
    if (!f) return -1L;
    fseek(f, 0, SEEK_END);
    long len = ftell(f);
    fseek(f, 0, SEEK_SET);
    if (!output) {
        fclose(f);
        return -1L;
    }
    *output = malloc(len);
    if (!*output) {
        fclose(f);
        return -1L;
    }
    len = (long) fread(*output, 1, len, f);
    (*output)[len] = 0;
    fclose(f);
    return len;
}

#define ASSERT(CHECK, ...) {if(!(CHECK))die(__VA_ARGS__);}
#define RETURN(CHECK) {if(!(CHECK))return false;}
#define VKRETURN(FUNC) {if(FUNC!=VK_SUCCESS)return false;}
#define VKCHECK(FUNC) {const VkResult RES=FUNC;if(RES!=VK_SUCCESS)die("VkResult(%d): "#FUNC, RES);}
static void die(const char *fmt, ...)
{
    va_list ap;
    va_start(ap, fmt);
    vfprintf(stderr, fmt, ap);
    va_end(ap);
    fputc('\n', stderr);
    fflush(stderr);
    SteamAPI_Shutdown();
    system("pause");
    exit(1);
}

struct WindowsObjects {
    HWND hwnd;
    double clock_freq;
} win;

internal LRESULT CALLBACK WindowProc(HWND hwnd, UINT msg, WPARAM wparam, LPARAM lparam)
{
    switch (msg) {
        case WM_KEYDOWN:
        case WM_KEYUP:
        case WM_SYSKEYDOWN:
        case WM_SYSKEYUP: {
            // const u16 repeat_count = LOWORD(lparam);
            const b32 isdown = !(lparam >> 31);
            // const b32 wasdown = ((lparam >> 30) & 1);
            const b32 isaltdown = (msg == WM_SYSKEYDOWN || msg == WM_SYSKEYUP) && (lparam & (1 << 29));
            switch (wparam) {
                case VK_ESCAPE: {
                    if (isdown)
                        PostQuitMessage(0);
                } break;
                case VK_F4: {
                    if (isdown && isaltdown)
                        PostQuitMessage(0);
                } break;
            }
        } break;
        case WM_SIZE: {
            // int w = LOWORD(lparam);
            // int h = HIWORD(lparam);
        } break;
        case WM_DESTROY: {
            PostQuitMessage(0);
        } break;
        default: return DefWindowProcA(hwnd, msg, wparam, lparam);
    }
    return 0;
}

static bool process_messages(void)
{
    MSG msg;
    while (PeekMessageA(&msg, NULL, 0, 0, PM_REMOVE)) {
        if (msg.message == WM_QUIT) {
            return false;
        }
        TranslateMessage(&msg);
        DispatchMessageA(&msg);
    }
    return true;
}

#define MAX_FRAMES_IN_FLIGHT 3
#define MAX_QUEUE_FAMILIES 2

struct VulkanObjects {
    VkInstance instance;
    VkSurfaceKHR surface;
    VkPhysicalDevice pdevice;
    i64 graphics_queue_family;
    i64 present_queue_family;
    VkDevice device;
    VkQueue graphics_queue;
    VkQueue present_queue;
    usize unique_queue_family_count;
    u32 unique_queue_families[MAX_QUEUE_FAMILIES];
    VkSurfaceCapabilitiesKHR caps;
    VkSurfaceFormatKHR format;
    VkPresentModeKHR present_mode;
    VkExtent2D swap_extent;
    u32 image_count;
    VkSwapchainKHR swapchain;
    VkImage images[10];
    VkImageView image_views[10];
    VkRenderPass renderpass;
    VkShaderModule vertmodule;
    VkShaderModule fragmodule;
    VkPipelineLayout pipeline_layout;
    VkPipeline pipeline;
    VkFramebuffer framebuffers[10];
    VkBuffer vertex_buffer;
    VkDeviceMemory vertex_buffer_memory;
    VkBuffer index_buffer;
    VkDeviceMemory index_buffer_memory;
    VkCommandPool commandpool;
    VkCommandBuffer commandbuffers[MAX_FRAMES_IN_FLIGHT];
    VkSemaphore image_available_semaphores[MAX_FRAMES_IN_FLIGHT];
    VkSemaphore render_finished_semaphores[MAX_FRAMES_IN_FLIGHT];
    VkFence in_flight_fences[MAX_FRAMES_IN_FLIGHT];
    u32 current_frame;
};

#define LENGTH(ARRAY) (sizeof(ARRAY) / sizeof((ARRAY)[0]))

struct Vertex {
    V2 position;
    V3 color;
};

const struct Vertex vertices[] = {
    {{-0.5f, -0.5f}, {1.0f, 0.0f, 0.0f}},
    {{0.5f, -0.5f}, {0.0f, 1.0f, 0.0f}},
    {{0.5f, 0.5f}, {0.0f, 0.0f, 1.0f}},
    {{-0.5f, 0.5f}, {1.0f, 1.0f, 1.0f}}
};

const u32 indices[] = {
    0, 1, 2, 2, 3, 0
};

static bool create_swapchain(struct VulkanObjects *vk)
{
    {
        VKRETURN(vkGetPhysicalDeviceSurfaceCapabilitiesKHR(vk->pdevice, vk->surface, &vk->caps));
    }
    {
        RETURN(vk->caps.currentExtent.width != UINT32_MAX);
        vk->swap_extent = vk->caps.currentExtent;
    }
    {
        vk->image_count = vk->caps.minImageCount + 1;
        if (vk->caps.maxImageCount != 0 && vk->image_count > vk->caps.maxImageCount) {
            vk->image_count = vk->caps.maxImageCount;
        }
    }

    VKRETURN(vkCreateSwapchainKHR(vk->device, &(VkSwapchainCreateInfoKHR) {
        .sType = VK_STRUCTURE_TYPE_SWAPCHAIN_CREATE_INFO_KHR,
        .surface = vk->surface,
        .minImageCount = vk->image_count,
        .imageFormat = vk->format.format,
        .imageColorSpace = vk->format.colorSpace,
        .imageExtent = vk->swap_extent,
        .imageArrayLayers = 1,
        .imageUsage = VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT,
        .imageSharingMode = vk->unique_queue_family_count == 1 ? VK_SHARING_MODE_EXCLUSIVE : VK_SHARING_MODE_CONCURRENT,
        .queueFamilyIndexCount = vk->unique_queue_family_count == 1 ? 0 : vk->unique_queue_family_count,
        .pQueueFamilyIndices = vk->unique_queue_family_count == 1 ? 0 : vk->unique_queue_families,
        .preTransform = vk->caps.currentTransform,
        .compositeAlpha = VK_COMPOSITE_ALPHA_OPAQUE_BIT_KHR,
        .presentMode = vk->present_mode,
        .clipped = VK_TRUE,
    }, NULL, &vk->swapchain));

    {
        RETURN(vk->image_count < 10);
        u32 starting_image_count = vk->image_count;
        VKRETURN(vkGetSwapchainImagesKHR(vk->device, vk->swapchain, &vk->image_count, vk->images));
        RETURN(starting_image_count == vk->image_count);
    }

    for (u32 i = 0; i < vk->image_count; ++i) {
        VKRETURN(vkCreateImageView(vk->device, &(VkImageViewCreateInfo) {
            .sType = VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO,
            .image = vk->images[i],
            .viewType = VK_IMAGE_VIEW_TYPE_2D,
            .format = vk->format.format,
            .components = {
                .r = VK_COMPONENT_SWIZZLE_IDENTITY,
                .g = VK_COMPONENT_SWIZZLE_IDENTITY,
                .b = VK_COMPONENT_SWIZZLE_IDENTITY,
                .a = VK_COMPONENT_SWIZZLE_IDENTITY,
            },
            .subresourceRange = {
                .aspectMask = VK_IMAGE_ASPECT_COLOR_BIT,
                .levelCount = 1,
                .layerCount = 1
            }
        }, NULL, vk->image_views+i));
    }

    VKRETURN(vkCreateRenderPass(vk->device, &(VkRenderPassCreateInfo) {
        .sType = VK_STRUCTURE_TYPE_RENDER_PASS_CREATE_INFO,
        .subpassCount = 1,
        .pSubpasses = &(VkSubpassDescription) {
            .pipelineBindPoint = VK_PIPELINE_BIND_POINT_GRAPHICS,
            .colorAttachmentCount = 1,
            .pColorAttachments = &(VkAttachmentReference) {
                .attachment = 0,
                .layout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL
            }
        },
        .attachmentCount = 1,
        .pAttachments = &(VkAttachmentDescription) {
            .format = vk->format.format,
            .samples = VK_SAMPLE_COUNT_1_BIT,
            .loadOp = VK_ATTACHMENT_LOAD_OP_CLEAR,
            .storeOp = VK_ATTACHMENT_STORE_OP_STORE,
            .stencilLoadOp = VK_ATTACHMENT_LOAD_OP_DONT_CARE,
            .stencilStoreOp = VK_ATTACHMENT_STORE_OP_DONT_CARE,
            .initialLayout = VK_IMAGE_LAYOUT_UNDEFINED,
            .finalLayout = VK_IMAGE_LAYOUT_PRESENT_SRC_KHR
        },
        .dependencyCount = 1,
        .pDependencies = &(VkSubpassDependency) {
            .srcSubpass = VK_SUBPASS_EXTERNAL,
            .dstSubpass = 0,
            .srcStageMask = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT,
            .srcAccessMask = 0,
            .dstStageMask = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT,
            .dstAccessMask = VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT
        }
    }, NULL, &vk->renderpass));

    // GRAPHICS PIPELINE STUFF FOLLOWS THIS
    {
        u8 *vertcode;
        const long vertlen = read_file("build/vert.spv", &vertcode);
        RETURN(vertlen > 0);
        u8 *fragcode;
        const long fraglen = read_file("build/frag.spv", &fragcode);
        RETURN(fraglen > 0);

        VKRETURN(vkCreateShaderModule(vk->device, &(VkShaderModuleCreateInfo) {
            .sType = VK_STRUCTURE_TYPE_SHADER_MODULE_CREATE_INFO,
            .codeSize = vertlen,
            .pCode = (const u32 *) vertcode
        }, NULL, &vk->vertmodule));

        VKRETURN(vkCreateShaderModule(vk->device, &(VkShaderModuleCreateInfo) {
            .sType = VK_STRUCTURE_TYPE_SHADER_MODULE_CREATE_INFO,
            .codeSize = fraglen,
            .pCode = (const u32 *) fragcode
        }, NULL, &vk->fragmodule));

        free(vertcode);
        free(fragcode);
    }

    VKRETURN(vkCreatePipelineLayout(vk->device, &(VkPipelineLayoutCreateInfo) {
        .sType = VK_STRUCTURE_TYPE_PIPELINE_LAYOUT_CREATE_INFO
    }, NULL, &vk->pipeline_layout));

    VKRETURN(vkCreateGraphicsPipelines(vk->device, VK_NULL_HANDLE, 1, &(VkGraphicsPipelineCreateInfo) {
        .sType = VK_STRUCTURE_TYPE_GRAPHICS_PIPELINE_CREATE_INFO,
        .stageCount = 2,
        .pStages = (VkPipelineShaderStageCreateInfo []) {
            {
                .sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO,
                .stage = VK_SHADER_STAGE_VERTEX_BIT,
                .module = vk->vertmodule,
                .pName = "main"
            },
            {
                .sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO,
                .stage = VK_SHADER_STAGE_FRAGMENT_BIT,
                .module = vk->fragmodule,
                .pName = "main"
            }
        },
        .pVertexInputState = &(VkPipelineVertexInputStateCreateInfo) {
            .sType = VK_STRUCTURE_TYPE_PIPELINE_VERTEX_INPUT_STATE_CREATE_INFO,
            .vertexBindingDescriptionCount = 1,
            .pVertexBindingDescriptions = &(VkVertexInputBindingDescription) {
                .binding = 0,
                .stride = sizeof(struct Vertex),
                .inputRate = VK_VERTEX_INPUT_RATE_VERTEX
            },
            .vertexAttributeDescriptionCount = 2,
            .pVertexAttributeDescriptions = (VkVertexInputAttributeDescription []) {
                {
                    .binding = 0,
                    .location = 0,
                    .format = VK_FORMAT_R32G32_SFLOAT,
                    .offset = offsetof(struct Vertex, position)
                },
                {
                    .binding = 0,
                    .location = 1,
                    .format = VK_FORMAT_R32G32B32_SFLOAT,
                    .offset = offsetof(struct Vertex, color)
                }
            }
        },
        .pInputAssemblyState = &(VkPipelineInputAssemblyStateCreateInfo) {
            .sType = VK_STRUCTURE_TYPE_PIPELINE_INPUT_ASSEMBLY_STATE_CREATE_INFO,
            .topology = VK_PRIMITIVE_TOPOLOGY_TRIANGLE_LIST
        },
        .pViewportState = &(VkPipelineViewportStateCreateInfo) {
            .sType = VK_STRUCTURE_TYPE_PIPELINE_VIEWPORT_STATE_CREATE_INFO,
            .viewportCount = 1,
            .pViewports = &(VkViewport) {
                .width = vk->swap_extent.width,
                .height = vk->swap_extent.height,
                .maxDepth = 1.0f
            },
            .scissorCount = 1,
            .pScissors = &(VkRect2D) {
                .extent = vk->swap_extent
            }
        },
        .pRasterizationState = &(VkPipelineRasterizationStateCreateInfo) {
            .sType = VK_STRUCTURE_TYPE_PIPELINE_RASTERIZATION_STATE_CREATE_INFO,
            .polygonMode = VK_POLYGON_MODE_FILL,
            .lineWidth = 1.0f,
            .cullMode = VK_CULL_MODE_BACK_BIT,
            .frontFace = VK_FRONT_FACE_CLOCKWISE
        },
        .pMultisampleState = &(VkPipelineMultisampleStateCreateInfo) {
            .sType = VK_STRUCTURE_TYPE_PIPELINE_MULTISAMPLE_STATE_CREATE_INFO,
            .rasterizationSamples = VK_SAMPLE_COUNT_1_BIT
        },
        .pDepthStencilState = NULL,
        .pColorBlendState = &(VkPipelineColorBlendStateCreateInfo) {
            .sType = VK_STRUCTURE_TYPE_PIPELINE_COLOR_BLEND_STATE_CREATE_INFO,
            .attachmentCount = 1,
            .pAttachments = &(VkPipelineColorBlendAttachmentState) {
                .colorWriteMask = VK_COLOR_COMPONENT_R_BIT|VK_COLOR_COMPONENT_G_BIT|VK_COLOR_COMPONENT_B_BIT|VK_COLOR_COMPONENT_A_BIT,
                .blendEnable = VK_TRUE,
                .srcColorBlendFactor = VK_BLEND_FACTOR_SRC_ALPHA,
                .dstColorBlendFactor = VK_BLEND_FACTOR_ONE_MINUS_SRC_ALPHA,
                .colorBlendOp = VK_BLEND_OP_ADD,
                .srcAlphaBlendFactor = VK_BLEND_FACTOR_ONE,
                .dstAlphaBlendFactor = VK_BLEND_FACTOR_ZERO,
                .alphaBlendOp = VK_BLEND_OP_ADD
            }
        },
        .pDynamicState = NULL,
        .layout = vk->pipeline_layout,
        .renderPass = vk->renderpass,
        .subpass = 0,
    }, NULL, &vk->pipeline));

    vkDestroyShaderModule(vk->device, vk->vertmodule, NULL);
    vkDestroyShaderModule(vk->device, vk->fragmodule, NULL);

    {
        for (u32 i = 0; i < vk->image_count; ++i) {
            VKRETURN(vkCreateFramebuffer(vk->device, &(VkFramebufferCreateInfo) {
                .sType = VK_STRUCTURE_TYPE_FRAMEBUFFER_CREATE_INFO,
                .renderPass = vk->renderpass,
                .attachmentCount = 1,
                .pAttachments = vk->image_views+i,
                .width = vk->swap_extent.width,
                .height = vk->swap_extent.height,
                .layers = 1
            }, NULL, vk->framebuffers+i));
        }
    }

    return true;
}

static void destroy_swapchain(struct VulkanObjects *vk)
{
    vkDeviceWaitIdle(vk->device);

    for (u32 i = 0; i < vk->image_count; ++i) {
        vkDestroyFramebuffer(vk->device, vk->framebuffers[i], NULL);
    }

    vkDestroyPipeline(vk->device, vk->pipeline, NULL);
    vkDestroyPipelineLayout(vk->device, vk->pipeline_layout, NULL);
    vkDestroyRenderPass(vk->device, vk->renderpass, NULL);

    for (u32 i = 0; i < vk->image_count; ++i) {
        vkDestroyImageView(vk->device, vk->image_views[i], NULL);
    }

    vkDestroySwapchainKHR(vk->device, vk->swapchain, NULL);
}

static bool IsDegenerate(HWND hwnd)
{
    RECT rect;
    GetClientRect(hwnd, &rect);
    const LONG w = rect.right - rect.left;
    const LONG h = rect.bottom - rect.top;
    return w == 0 || h == 0;
}

static bool recreate_swapchain(struct VulkanObjects *vk)
{
    while (IsIconic(win.hwnd) || IsDegenerate(win.hwnd)) {
        if (!process_messages())
            return false;
        Sleep(100);
    }
    destroy_swapchain(vk);
    return create_swapchain(vk);
}

static void record_command_buffer(const struct VulkanObjects *vk, VkCommandBuffer buffer, u32 image_index)
{
    VKCHECK(vkBeginCommandBuffer(buffer, &(VkCommandBufferBeginInfo) {
        .sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO
    }));
    vkCmdBeginRenderPass(buffer, &(VkRenderPassBeginInfo) {
        .sType = VK_STRUCTURE_TYPE_RENDER_PASS_BEGIN_INFO,
        .renderPass = vk->renderpass,
        .framebuffer = vk->framebuffers[image_index],
        .renderArea.extent = vk->swap_extent,
        .clearValueCount = 1,
        .pClearValues = &(VkClearValue) {.color = {.float32 = {0.2f, 0.3f, 0.8f, 1.0f}}}
    }, VK_SUBPASS_CONTENTS_INLINE);
    vkCmdBindPipeline(buffer, VK_PIPELINE_BIND_POINT_GRAPHICS, vk->pipeline);
    vkCmdBindVertexBuffers(buffer, 0, 1, &vk->vertex_buffer, &(VkDeviceSize) {0});
    vkCmdBindIndexBuffer(buffer, vk->index_buffer, 0, VK_INDEX_TYPE_UINT32);
    vkCmdDrawIndexed(buffer, LENGTH(indices), 1, 0, 0, 0);
    vkCmdEndRenderPass(buffer);
    VKCHECK(vkEndCommandBuffer(buffer));
}

static void draw_frame(struct VulkanObjects *vk)
{
    vkWaitForFences(vk->device, 1, vk->in_flight_fences+vk->current_frame, VK_TRUE, UINT64_MAX);

    u32 image_index;
    VkResult result = vkAcquireNextImageKHR(vk->device, vk->swapchain, UINT64_MAX, vk->image_available_semaphores[vk->current_frame], VK_NULL_HANDLE, &image_index);
    if (result == VK_ERROR_OUT_OF_DATE_KHR) {
        fprintf(stderr, "recreating swapchain because acquire failed\n");
        if (!recreate_swapchain(vk)) {
            PostQuitMessage(0);
        }
        return;
    }
    ASSERT(result == VK_SUCCESS || result == VK_SUBOPTIMAL_KHR, "failed to acquire swap chain image");

    vkResetFences(vk->device, 1, vk->in_flight_fences+vk->current_frame);

    vkResetCommandBuffer(vk->commandbuffers[vk->current_frame], 0);
    record_command_buffer(vk, vk->commandbuffers[vk->current_frame], image_index);
    VKCHECK(vkQueueSubmit(vk->graphics_queue, 1, &(VkSubmitInfo) {
        .sType = VK_STRUCTURE_TYPE_SUBMIT_INFO,
        .waitSemaphoreCount = 1,
        .pWaitSemaphores = vk->image_available_semaphores+vk->current_frame,
        .pWaitDstStageMask = &(VkPipelineStageFlags) {VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT},
        .commandBufferCount = 1,
        .pCommandBuffers = vk->commandbuffers+vk->current_frame,
        .signalSemaphoreCount = 1,
        .pSignalSemaphores = vk->render_finished_semaphores+vk->current_frame
    }, vk->in_flight_fences[vk->current_frame]));

    result = vkQueuePresentKHR(vk->present_queue, &(VkPresentInfoKHR) {
        .sType = VK_STRUCTURE_TYPE_PRESENT_INFO_KHR,
        .waitSemaphoreCount = 1,
        .pWaitSemaphores = vk->render_finished_semaphores+vk->current_frame,
        .swapchainCount = 1,
        .pSwapchains = &vk->swapchain,
        .pImageIndices = &image_index
    });
    if (result == VK_ERROR_OUT_OF_DATE_KHR || result == VK_SUBOPTIMAL_KHR) {
        fprintf(stderr, "recreating swapchain because present failed\n");
        if (!recreate_swapchain(vk)) {
            PostQuitMessage(0);
        }
    } else {
        ASSERT(result == VK_SUCCESS, "failed to present swap chain image");
    }

    vk->current_frame = (vk->current_frame + 1) % MAX_FRAMES_IN_FLIGHT;
}

static u32 find_mem_type(const struct VulkanObjects *vk, u32 type_filter, VkMemoryPropertyFlags properties)
{
    VkPhysicalDeviceMemoryProperties mem_props;
    vkGetPhysicalDeviceMemoryProperties(vk->pdevice, &mem_props);

    for (u32 i = 0; i < mem_props.memoryTypeCount; ++i) {
        if ((type_filter & (1 << i)) &&
            (mem_props.memoryTypes[i].propertyFlags & properties) == properties) {
            return i;
        }
    }

    ASSERT(false, "failed to find suitable memory type");
    return UINT32_MAX;
}

static bool create_buffer(const struct VulkanObjects *vk, VkDeviceSize size, VkBufferUsageFlags usage, VkMemoryPropertyFlags properties, VkBuffer *buffer, VkDeviceMemory *buffer_memory)
{
    VKRETURN(vkCreateBuffer(vk->device, &(VkBufferCreateInfo) {
        .sType = VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO,
        .size = size,
        .usage = usage,
        .sharingMode = VK_SHARING_MODE_EXCLUSIVE
    }, NULL, buffer));
    {
        VkMemoryRequirements mem_reqs;
        vkGetBufferMemoryRequirements(vk->device, *buffer, &mem_reqs);
        VKRETURN(vkAllocateMemory(vk->device, &(VkMemoryAllocateInfo) {
            .sType = VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO,
            .allocationSize = mem_reqs.size,
            .memoryTypeIndex = find_mem_type(vk, mem_reqs.memoryTypeBits, properties)
        }, NULL, buffer_memory));
        VKRETURN(vkBindBufferMemory(vk->device, *buffer, *buffer_memory, 0));
    }
    return true;
}

static bool copy_buffer(const struct VulkanObjects *vk, VkBuffer src, VkBuffer dst, VkDeviceSize size)
{
    VkCommandBuffer commandbuffer;
    VKRETURN(vkAllocateCommandBuffers(vk->device, &(VkCommandBufferAllocateInfo) {
        .sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO,
        .level = VK_COMMAND_BUFFER_LEVEL_PRIMARY,
        .commandPool = vk->commandpool,
        .commandBufferCount = 1
    }, &commandbuffer));

    VKRETURN(vkBeginCommandBuffer(commandbuffer, &(VkCommandBufferBeginInfo) {
        .sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO,
        .flags = VK_COMMAND_BUFFER_USAGE_ONE_TIME_SUBMIT_BIT
    }));

    vkCmdCopyBuffer(commandbuffer, src, dst, 1, &(VkBufferCopy) {
        .size = size
    });

    VKRETURN(vkEndCommandBuffer(commandbuffer));

    VKRETURN(vkQueueSubmit(vk->graphics_queue, 1, &(VkSubmitInfo) {
        .sType = VK_STRUCTURE_TYPE_SUBMIT_INFO,
        .commandBufferCount = 1,
        .pCommandBuffers = &commandbuffer
    }, VK_NULL_HANDLE));

    VKRETURN(vkQueueWaitIdle(vk->graphics_queue));

    vkFreeCommandBuffers(vk->device, vk->commandpool, 1, &commandbuffer);

    return true;
}

static i64 get_current_time(void)
{
    LARGE_INTEGER li;
    QueryPerformanceCounter(&li);
    return li.QuadPart;
}

static double get_time_since(i64 since)
{
    LARGE_INTEGER li;
    QueryPerformanceCounter(&li);
    return (li.QuadPart - since) / win.clock_freq;
}

#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wunused-parameter"
int WINAPI WinMain(HINSTANCE inst, HINSTANCE prev, PSTR cmdline, INT show)
#pragma GCC diagnostic pop
{
    if (SteamAPI_RestartAppIfNecessary(480)) {
        return 1;
    }
    if (CSteamAPI_Init()) {
        const uint64_steamid id = SteamUser.GetSteamID();
        const char *name = SteamFriends.GetPersonaName();
        const int num_friends = SteamFriends.GetFriendCount(k_EFriendFlagAll);
        const int img = SteamFriends.GetLargeFriendAvatar(id);
        if (img > 0) {
            u32 w, h;
            bool success = SteamUtils.GetImageSize(img, &w, &h);
            ASSERT(success, "failed to get image size");
            const int size_bytes = w * h * 4;
            u8 *rgba_image = malloc(size_bytes);
            success = SteamUtils.GetImageRGBA(img, rgba_image, size_bytes);
            ASSERT(success, "failed to get image data");
            // the image is in rgba_image!
            printf("Image loaded\n");
            free(rgba_image);
        }

        printf("Steam Name: %s (%llu)\nNum Friends: %d\n", name, id, num_friends);
    }

    {
        LARGE_INTEGER li;
        QueryPerformanceFrequency(&li);
        win.clock_freq = li.QuadPart;
    }
    const WNDCLASSA wc = {
        .style = CS_OWNDC|CS_HREDRAW|CS_VREDRAW,
        .lpfnWndProc = WindowProc,
        .hInstance = inst,
        .lpszClassName = "GameWindowClass"
    };
    RegisterClassA(&wc);
    win.hwnd = CreateWindowExA(
        0,
        wc.lpszClassName,
        "Window Title",
        WS_OVERLAPPEDWINDOW|WS_VISIBLE,
        CW_USEDEFAULT, CW_USEDEFAULT, 800, 600,
        NULL,
        NULL,
        inst,
        NULL
    );

    struct VulkanObjects vk;

    VKCHECK(vkCreateInstance(&(VkInstanceCreateInfo) {
        .sType = VK_STRUCTURE_TYPE_INSTANCE_CREATE_INFO,
        .pApplicationInfo = &(VkApplicationInfo) {
            .sType = VK_STRUCTURE_TYPE_APPLICATION_INFO,
            .apiVersion = VK_API_VERSION_1_0,
        },
        .enabledExtensionCount = 2,
        .ppEnabledExtensionNames = (const char *[]) {VK_KHR_SURFACE_EXTENSION_NAME, VK_KHR_WIN32_SURFACE_EXTENSION_NAME},
        .enabledLayerCount = 1,
        .ppEnabledLayerNames = (const char *[]) {"VK_LAYER_KHRONOS_validation"}
    }, NULL, &vk.instance));

    VKCHECK(vkCreateWin32SurfaceKHR(vk.instance, &(VkWin32SurfaceCreateInfoKHR) {
        .sType = VK_STRUCTURE_TYPE_WIN32_SURFACE_CREATE_INFO_KHR,
        .hwnd = win.hwnd,
        .hinstance = inst
    }, NULL, &vk.surface));

    {
        u32 count = 1;
        VKCHECK(vkEnumeratePhysicalDevices(vk.instance, &count, &vk.pdevice));
        ASSERT(count == 1, "failed to find vulkan supported gpu");
    }

    {
        u32 count = 10;
        VkQueueFamilyProperties props[10];
        vkGetPhysicalDeviceQueueFamilyProperties(vk.pdevice, &count, props);
        vk.graphics_queue_family = -1;
        vk.present_queue_family = -1;
        for (u32 i = 0; i < count; ++i) {
            if (props[i].queueFlags & VK_QUEUE_GRAPHICS_BIT) {
                vk.graphics_queue_family = i;
            }

            VkBool32 present = VK_FALSE;
            VKCHECK(vkGetPhysicalDeviceSurfaceSupportKHR(vk.pdevice, i, vk.surface, &present));
            if (present == VK_TRUE) {
                vk.present_queue_family = i;
            }
        }
        ASSERT(vk.graphics_queue_family != -1, "failed to find graphics queue");
        ASSERT(vk.present_queue_family != -1, "failed to find present queue");
    }

    const u32 all_queue_families[MAX_QUEUE_FAMILIES] = {vk.graphics_queue_family, vk.present_queue_family};
    vk.unique_queue_family_count = 0;
    for (usize i = 0; i < MAX_QUEUE_FAMILIES; ++i) {
        b32 inside = false;
        for (usize j = 0; j < vk.unique_queue_family_count; ++j) {
            if (vk.unique_queue_families[j] == all_queue_families[i]) {
                inside = true;
            }
        }
        if (!inside) {
            vk.unique_queue_families[vk.unique_queue_family_count++] = all_queue_families[i];
        }
    }
    VkDeviceQueueCreateInfo queue_infos[MAX_QUEUE_FAMILIES];
    for (usize i = 0; i < vk.unique_queue_family_count; ++i) {
        queue_infos[i] = (VkDeviceQueueCreateInfo) {
            .sType = VK_STRUCTURE_TYPE_DEVICE_QUEUE_CREATE_INFO,
            .queueFamilyIndex = vk.unique_queue_families[i],
            .queueCount = 1,
            .pQueuePriorities = (float []) {1.0f}
        };
    }

    VKCHECK(vkCreateDevice(vk.pdevice, &(VkDeviceCreateInfo) {
        .sType = VK_STRUCTURE_TYPE_DEVICE_CREATE_INFO,
        .queueCreateInfoCount = vk.unique_queue_family_count,
        .pQueueCreateInfos = queue_infos,
        .pEnabledFeatures = &(VkPhysicalDeviceFeatures) {
            VK_FALSE
        },
        .enabledExtensionCount = 1,
        .ppEnabledExtensionNames = (const char *[]) {VK_KHR_SWAPCHAIN_EXTENSION_NAME}
    }, NULL, &vk.device));

    {
        vkGetDeviceQueue(vk.device, vk.graphics_queue_family, 0, &vk.graphics_queue);
        vkGetDeviceQueue(vk.device, vk.present_queue_family, 0, &vk.present_queue);
    }

    // THE FOLLOWING IS SWAPCHAIN STUFFS SMILE
    {
        u32 count = 20;
        VkSurfaceFormatKHR formats[20];
        VKCHECK(vkGetPhysicalDeviceSurfaceFormatsKHR(vk.pdevice, vk.surface, &count, formats));
        vk.format = formats[0];
        for (u32 i = 0; i < count; ++i) {
            if (formats[i].format == VK_FORMAT_B8G8R8A8_SRGB && formats[i].colorSpace == VK_COLOR_SPACE_SRGB_NONLINEAR_KHR) {
                vk.format = formats[i];
            }
        }
    }
    {
        u32 count = 20;
        VkPresentModeKHR modes[20];
        VKCHECK(vkGetPhysicalDeviceSurfacePresentModesKHR(vk.pdevice, vk.surface, &count, modes));
        vk.present_mode = VK_PRESENT_MODE_FIFO_KHR;
        for (u32 i = 0; i < count; ++i) {
            if (modes[i] == VK_PRESENT_MODE_IMMEDIATE_KHR) {
                vk.present_mode = modes[i];
            }
            if (modes[i] == VK_PRESENT_MODE_MAILBOX_KHR) {
                vk.present_mode = modes[i];
                break;
            }
        }
    }

    ASSERT(create_swapchain(&vk), "failed to create swapchain");

    VKCHECK(vkCreateCommandPool(vk.device, &(VkCommandPoolCreateInfo) {
        .sType = VK_STRUCTURE_TYPE_COMMAND_POOL_CREATE_INFO,
        .flags = VK_COMMAND_POOL_CREATE_RESET_COMMAND_BUFFER_BIT,
        .queueFamilyIndex = vk.graphics_queue_family
    }, NULL, &vk.commandpool));

    {
        VkBuffer staging_buffer;
        VkDeviceMemory staging_buffer_memory;
        ASSERT(create_buffer(&vk, sizeof(vertices), VK_BUFFER_USAGE_TRANSFER_SRC_BIT, VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT|VK_MEMORY_PROPERTY_HOST_COHERENT_BIT, &staging_buffer, &staging_buffer_memory), "failed to create vertex staging buffer");

        void *data;
        VKCHECK(vkMapMemory(vk.device, staging_buffer_memory, 0, sizeof(vertices), 0, &data));
            memcpy(data, vertices, sizeof(vertices));
        vkUnmapMemory(vk.device, staging_buffer_memory);

        ASSERT(create_buffer(&vk, sizeof(vertices), VK_BUFFER_USAGE_TRANSFER_DST_BIT|VK_BUFFER_USAGE_VERTEX_BUFFER_BIT, VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT, &vk.vertex_buffer, &vk.vertex_buffer_memory), "failed to create vertex buffer");
        ASSERT(copy_buffer(&vk, staging_buffer, vk.vertex_buffer, sizeof(vertices)), "failed to copy from staging to vertex buffer");

        vkDestroyBuffer(vk.device, staging_buffer, NULL);
        vkFreeMemory(vk.device, staging_buffer_memory, NULL);
    }
    {
        VkBuffer staging_buffer;
        VkDeviceMemory staging_buffer_memory;
        ASSERT(create_buffer(&vk, sizeof(indices), VK_BUFFER_USAGE_TRANSFER_SRC_BIT, VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT|VK_MEMORY_PROPERTY_HOST_COHERENT_BIT, &staging_buffer, &staging_buffer_memory), "failed to create index staging buffer");

        void *data;
        VKCHECK(vkMapMemory(vk.device, staging_buffer_memory, 0, sizeof(indices), 0, &data));
            memcpy(data, indices, sizeof(indices));
        vkUnmapMemory(vk.device, staging_buffer_memory);

        ASSERT(create_buffer(&vk, sizeof(indices), VK_BUFFER_USAGE_TRANSFER_DST_BIT|VK_BUFFER_USAGE_INDEX_BUFFER_BIT, VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT, &vk.index_buffer, &vk.index_buffer_memory), "failed to create index buffer");
        ASSERT(copy_buffer(&vk, staging_buffer, vk.index_buffer, sizeof(indices)), "failed to copy from staging to index buffer");

        vkDestroyBuffer(vk.device, staging_buffer, NULL);
        vkFreeMemory(vk.device, staging_buffer_memory, NULL);
    }

    VKCHECK(vkAllocateCommandBuffers(vk.device, &(VkCommandBufferAllocateInfo) {
        .sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO,
        .commandPool = vk.commandpool,
        .level = VK_COMMAND_BUFFER_LEVEL_PRIMARY,
        .commandBufferCount = MAX_FRAMES_IN_FLIGHT
    }, vk.commandbuffers));

    {
        for (usize i = 0; i < MAX_FRAMES_IN_FLIGHT; ++i) {
            VKCHECK(vkCreateSemaphore(vk.device, &(VkSemaphoreCreateInfo) {
                .sType = VK_STRUCTURE_TYPE_SEMAPHORE_CREATE_INFO
            }, NULL, vk.image_available_semaphores+i));
            VKCHECK(vkCreateSemaphore(vk.device, &(VkSemaphoreCreateInfo) {
                .sType = VK_STRUCTURE_TYPE_SEMAPHORE_CREATE_INFO
            }, NULL, vk.render_finished_semaphores+i));
            VKCHECK(vkCreateFence(vk.device, &(VkFenceCreateInfo) {
                .sType = VK_STRUCTURE_TYPE_FENCE_CREATE_INFO,
                .flags = VK_FENCE_CREATE_SIGNALED_BIT
            }, NULL, vk.in_flight_fences+i));
        }
    }

    const i64 start_time = get_current_time();
    double last_update_time = 0.0;

    double last_fps_time = 0.0;
    int fps = 0;

    vk.current_frame = 0;
    while (process_messages()) {
        const double current_time = get_time_since(start_time);
        const double delta = current_time - last_update_time;
        last_update_time = current_time;

        last_fps_time += delta;
        ++fps;

        if (last_fps_time >= 1.0) {
            printf("%f ms/f - %d fps\n", 1000.0 / fps, fps);
            last_fps_time = 0.0;
            fps = 0;
        }

        draw_frame(&vk);
    }

    destroy_swapchain(&vk);
    for (usize i = 0; i < MAX_FRAMES_IN_FLIGHT; ++i) {
        vkDestroyFence(vk.device, vk.in_flight_fences[i], NULL);
        vkDestroySemaphore(vk.device, vk.render_finished_semaphores[i], NULL);
        vkDestroySemaphore(vk.device, vk.image_available_semaphores[i], NULL);
    }

    vkDestroyCommandPool(vk.device, vk.commandpool, NULL);

    vkDestroyBuffer(vk.device, vk.index_buffer, NULL);
    vkFreeMemory(vk.device, vk.index_buffer_memory, NULL);
    vkDestroyBuffer(vk.device, vk.vertex_buffer, NULL);
    vkFreeMemory(vk.device, vk.vertex_buffer_memory, NULL);

    vkDestroyDevice(vk.device, NULL);
    vkDestroySurfaceKHR(vk.instance, vk.surface, NULL);
    vkDestroyInstance(vk.instance, NULL);

    SteamAPI_Shutdown();
    system("pause");
    return 0;
}
