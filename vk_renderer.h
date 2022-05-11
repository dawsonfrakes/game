#ifndef VK_RENDERER
#define VK_RENDERER

#include "log.h"
#include "gametypes.h"
#include "fmath.h"
#include <vulkan/vulkan_core.h>

#if defined(_WIN32)
#define WIN32_LEAN_AND_MEAN
#define VK_USE_PLATFORM_WIN32_KHR
#define CURRENT_SURFACE_EXTENSION_NAME VK_KHR_WIN32_SURFACE_EXTENSION_NAME
#define RENDER_INFO_MEMBERS HINSTANCE inst; HWND hwnd;
#elif defined(__linux__)
#define VK_USE_PLATFORM_XLIB_KHR
#define CURRENT_SURFACE_EXTENSION_NAME VK_KHR_XLIB_SURFACE_EXTENSION_NAME
#define RENDER_INFO_MEMBERS Display *dpy; Window win;
#else
#error window protocol not defined
#endif /* _WIN32, __linux__ */

#include <vulkan/vulkan.h>

struct WindowToRendererInfo {
    RENDER_INFO_MEMBERS
};

// hard-coded temp values
typedef struct Vertex {
    V3 position;
    V3 color;
} Vertex;

const Vertex vertices[] = {
    {{0.0f, -0.5f}, {1.0f, 0.0f, 0.0f}},
    {{0.5f, 0.5f}, {0.0f, 1.0f, 0.0f}},
    {{-0.5f, 0.5f}, {0.0f, 0.0f, 1.0f}}
};

bool renderer_init(struct WindowToRendererInfo *win);
void renderer_update(void);
void renderer_deinit(void);

#ifdef INCLUDE_SRC

static long read_file(const char *filename, u8 **output)
{
    if (!output) return -1;
    FILE *f = fopen(filename, "rb");
    if (!f) return -1;
    fseek(f, 0, SEEK_END);
    long len = ftell(f);
    fseek(f, 0, SEEK_SET);
    *output = (u8 *) malloc(sizeof(u8) * (len + 1));
    if (!*output) {
        fclose(f);
        return -1;
    }
    len = fread(*output, sizeof(u8), len, f);
    (*output)[len] = '\0';
    fclose(f);
    return len;
}

#define VKRETURN(FUNC) {const VkResult R=FUNC;if(R!=VK_SUCCESS){LOGERROR("%s:%d: VkResult(%d): "#FUNC, __FILE__, __LINE__, R);return false;}}
#define VKASSERT(CHECK, ...) {if (!(CHECK)){LOGERROR(__VA_ARGS__);return false;}}

#define MAX_UNIQUE_QUEUE_FAMILIES 2
#define MAX_IMAGES 10
#define MAX_FRAMES_IN_FLIGHT 3

global struct VulkanObjects {
    VkInstance instance;
    VkSurfaceKHR surface;
    VkPhysicalDevice pdevice;
    i64 graphics_family_index, present_family_index;
    u32 unique_family_indices[MAX_UNIQUE_QUEUE_FAMILIES];
    usize unique_family_indices_count;
    VkDevice device;
    VkQueue graphics_queue, present_queue;
    VkSurfaceCapabilitiesKHR caps;
    VkSurfaceFormatKHR format;
    VkPresentModeKHR present_mode;
    VkExtent2D extent;
    u32 image_count;
    VkSwapchainKHR swapchain;
    VkImage swapchain_images[MAX_IMAGES];
    VkImageView swapchain_image_views[MAX_IMAGES];
    VkRenderPass renderpass;
    VkPipelineLayout graphics_pipeline_layout;
    VkPipeline graphics_pipeline;
    VkFramebuffer swapchain_framebuffers[MAX_IMAGES];
    VkCommandPool commandpool;
    VkBuffer vertex_buffer;
    VkDeviceMemory vertex_buffer_memory;
    VkCommandBuffer commandbuffers[MAX_FRAMES_IN_FLIGHT];
    VkSemaphore image_available_semaphores[MAX_FRAMES_IN_FLIGHT];
    VkSemaphore render_complete_semaphores[MAX_FRAMES_IN_FLIGHT];
    VkFence in_flight_fences[MAX_FRAMES_IN_FLIGHT];
} vk;

static bool swapchain_init(void)
{
    {
        VKRETURN(vkGetPhysicalDeviceSurfaceCapabilitiesKHR(vk.pdevice, vk.surface, &vk.caps));
    }
    {
        #define MAX_SURFACE_FORMATS 16
        u32 count = MAX_SURFACE_FORMATS;
        VkSurfaceFormatKHR formats[MAX_SURFACE_FORMATS];
        #undef MAX_SURFACE_FORMATS
        VKRETURN(vkGetPhysicalDeviceSurfaceFormatsKHR(vk.pdevice, vk.surface, &count, formats));
        VKASSERT(count > 0, "failed to find any surface formats");
        vk.format = formats[0];
        for (u32 i = 0; i < count; ++i) {
            if (formats[i].format == VK_FORMAT_B8G8R8A8_SRGB &&
                    formats[i].colorSpace == VK_COLOR_SPACE_SRGB_NONLINEAR_KHR) {
                vk.format = formats[i];
                break;
            }
        }
    }
    {
        #define MAX_PRESENT_MODES 16
        u32 count = MAX_PRESENT_MODES;
        VkPresentModeKHR present_modes[MAX_PRESENT_MODES];
        #undef MAX_SURFACE_FORMATS
        VKRETURN(vkGetPhysicalDeviceSurfacePresentModesKHR(vk.pdevice, vk.surface, &count, present_modes));
        vk.present_mode = VK_PRESENT_MODE_FIFO_KHR;
        for (u32 i = 0; i < count; ++i) {
            // if immediate exists, choose it (we hate vsync)
            if (present_modes[i] == VK_PRESENT_MODE_IMMEDIATE_KHR) {
                vk.present_mode = VK_PRESENT_MODE_IMMEDIATE_KHR;
                break;
            }

            // if mailbox exists, choose it over fifo
            if (present_modes[i] == VK_PRESENT_MODE_MAILBOX_KHR) {
                vk.present_mode = VK_PRESENT_MODE_MAILBOX_KHR;
            }
        }
    }
    {
        // TODO: get width and height from WM if caps doesn't have it automatically
        VKASSERT(vk.caps.currentExtent.width != UINT32_MAX, "can't retrieve window dimensions");
        vk.extent = vk.caps.currentExtent;
    }
    {
        vk.image_count = vk.caps.minImageCount + 1;
        if (vk.caps.maxImageCount > 0 && vk.image_count > vk.caps.maxImageCount) {
            vk.image_count = vk.caps.maxImageCount;
        }
        VKASSERT(vk.image_count <= MAX_IMAGES, "requested too many images");
    }
    {
        VKRETURN(vkCreateSwapchainKHR(vk.device, &(VkSwapchainCreateInfoKHR) {
            .sType = VK_STRUCTURE_TYPE_SWAPCHAIN_CREATE_INFO_KHR,
            .surface = vk.surface,
            .minImageCount = vk.image_count,
            .imageFormat = vk.format.format,
            .imageColorSpace = vk.format.colorSpace,
            .imageExtent = vk.extent,
            .imageArrayLayers = 1,
            .imageUsage = VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT,
            .imageSharingMode = vk.graphics_family_index != vk.present_family_index ? VK_SHARING_MODE_CONCURRENT : VK_SHARING_MODE_EXCLUSIVE,
            .queueFamilyIndexCount = vk.graphics_family_index != vk.present_family_index ? 2 : 0,
            .pQueueFamilyIndices = (u32 []) {vk.graphics_family_index, vk.present_family_index},
            .preTransform = vk.caps.currentTransform,
            .compositeAlpha = VK_COMPOSITE_ALPHA_OPAQUE_BIT_KHR,
            .presentMode = vk.present_mode,
            .clipped = VK_TRUE
        }, NULL, &vk.swapchain));
    }
    {
        const u32 starting_image_count = vk.image_count;
        VKRETURN(vkGetSwapchainImagesKHR(vk.device, vk.swapchain, &vk.image_count, vk.swapchain_images));
        if (starting_image_count != vk.image_count) {
            LOGWARN("Images: only %d of %d requested were acquired", vk.image_count, starting_image_count);
        }
    }
    {
        for (u32 i = 0; i < vk.image_count; ++i) {
            VKRETURN(vkCreateImageView(vk.device, &(VkImageViewCreateInfo) {
                .sType = VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO,
                .image = vk.swapchain_images[i],
                .viewType = VK_IMAGE_VIEW_TYPE_2D,
                .format = vk.format.format,
                .components = {
                    .r = VK_COMPONENT_SWIZZLE_IDENTITY,
                    .g = VK_COMPONENT_SWIZZLE_IDENTITY,
                    .b = VK_COMPONENT_SWIZZLE_IDENTITY,
                    .a = VK_COMPONENT_SWIZZLE_IDENTITY
                },
                .subresourceRange = {
                    .aspectMask = VK_IMAGE_ASPECT_COLOR_BIT,
                    .levelCount = 1,
                    .layerCount = 1
                }
            }, NULL, vk.swapchain_image_views+i));
        }
    }

    VKRETURN(vkCreateRenderPass(vk.device, &(VkRenderPassCreateInfo) {
        .sType = VK_STRUCTURE_TYPE_RENDER_PASS_CREATE_INFO,
        .attachmentCount = 1,
        .pAttachments = &(VkAttachmentDescription) {
            .format = vk.format.format,
            .samples = VK_SAMPLE_COUNT_1_BIT,
            .loadOp = VK_ATTACHMENT_LOAD_OP_CLEAR,
            .storeOp = VK_ATTACHMENT_STORE_OP_STORE,
            .stencilLoadOp = VK_ATTACHMENT_LOAD_OP_DONT_CARE,
            .stencilStoreOp = VK_ATTACHMENT_STORE_OP_DONT_CARE,
            .initialLayout = VK_IMAGE_LAYOUT_UNDEFINED,
            .finalLayout = VK_IMAGE_LAYOUT_PRESENT_SRC_KHR
        },
        .subpassCount = 1,
        .pSubpasses = &(VkSubpassDescription) {
            .pipelineBindPoint = VK_PIPELINE_BIND_POINT_GRAPHICS,
            .colorAttachmentCount = 1,
            .pColorAttachments = &(VkAttachmentReference) {
                .attachment = 0,
                .layout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL
            }
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
    }, NULL, &vk.renderpass));

    // GRAPHICS PIPELINE

    // create shader modules
    VkShaderModule vertex_module;
    VkShaderModule fragment_module;
    {
        u8 *vertex_code, *fragment_code;
        const long vertex_code_len = read_file("build/vert.spv", &vertex_code);
        const long fragment_code_len = read_file("build/frag.spv", &fragment_code);
        VKASSERT(vertex_code_len != -1, "failed to read vertex file");
        VKASSERT(fragment_code_len != -1, "failed to read fragment file");

        VKRETURN(vkCreateShaderModule(vk.device, &(VkShaderModuleCreateInfo) {
            .sType = VK_STRUCTURE_TYPE_SHADER_MODULE_CREATE_INFO,
            .codeSize = vertex_code_len,
            .pCode = (const u32 *) vertex_code
        }, NULL, &vertex_module));

        VKRETURN(vkCreateShaderModule(vk.device, &(VkShaderModuleCreateInfo) {
            .sType = VK_STRUCTURE_TYPE_SHADER_MODULE_CREATE_INFO,
            .codeSize = fragment_code_len,
            .pCode = (const u32 *) fragment_code
        }, NULL, &fragment_module));

        free(vertex_code);
        free(fragment_code);
    }

    VKRETURN(vkCreatePipelineLayout(vk.device, &(VkPipelineLayoutCreateInfo) {
        .sType = VK_STRUCTURE_TYPE_PIPELINE_LAYOUT_CREATE_INFO
    }, NULL, &vk.graphics_pipeline_layout));

    VKRETURN(vkCreateGraphicsPipelines(vk.device, VK_NULL_HANDLE, 1, &(VkGraphicsPipelineCreateInfo) {
        .sType = VK_STRUCTURE_TYPE_GRAPHICS_PIPELINE_CREATE_INFO,
        .stageCount = 2,
        .pStages = (VkPipelineShaderStageCreateInfo []) {
            {
                .sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO,
                .stage = VK_SHADER_STAGE_VERTEX_BIT,
                .module = vertex_module,
                .pName = "main"
            },
            {
                .sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO,
                .stage = VK_SHADER_STAGE_FRAGMENT_BIT,
                .module = fragment_module,
                .pName = "main"
            }
        },
        .pVertexInputState = &(VkPipelineVertexInputStateCreateInfo) {
            .sType = VK_STRUCTURE_TYPE_PIPELINE_VERTEX_INPUT_STATE_CREATE_INFO,
            .vertexBindingDescriptionCount = 1,
            .vertexAttributeDescriptionCount = 2,
            .pVertexBindingDescriptions = &(VkVertexInputBindingDescription) {
                .binding = 0,
                .stride = sizeof(Vertex),
                .inputRate = VK_VERTEX_INPUT_RATE_VERTEX
            },
            .pVertexAttributeDescriptions = (VkVertexInputAttributeDescription []) {
                {
                    .binding = 0,
                    .location = 0,
                    .format = VK_FORMAT_R32G32B32_SFLOAT,
                    .offset = offsetof(Vertex, position)
                },
                {
                    .binding = 0,
                    .location = 1,
                    .format = VK_FORMAT_R32G32B32_SFLOAT,
                    .offset = offsetof(Vertex, color)
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
                .width = vk.extent.width,
                .height = vk.extent.height,
                .maxDepth = 1.0f
            },
            .scissorCount = 1,
            .pScissors = &(VkRect2D) {
                .extent = vk.extent
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
        .layout = vk.graphics_pipeline_layout,
        .renderPass = vk.renderpass,
        .subpass = 0
    }, NULL, &vk.graphics_pipeline));

    vkDestroyShaderModule(vk.device, vertex_module, NULL);
    vkDestroyShaderModule(vk.device, fragment_module, NULL);

    // framebuffers
    {
        for (u32 i = 0; i < vk.image_count; ++i) {
            VKRETURN(vkCreateFramebuffer(vk.device, &(VkFramebufferCreateInfo) {
                .sType = VK_STRUCTURE_TYPE_FRAMEBUFFER_CREATE_INFO,
                .renderPass = vk.renderpass,
                .attachmentCount = 1,
                .pAttachments = vk.swapchain_image_views+i,
                .width = vk.extent.width,
                .height = vk.extent.height,
                .layers = 1
            }, NULL, vk.swapchain_framebuffers+i));
        }
    }
    return true;
}

static void swapchain_deinit(void)
{
    vkDeviceWaitIdle(vk.device);
    for (u32 i = 0; i < vk.image_count; ++i) {
        vkDestroyFramebuffer(vk.device, vk.swapchain_framebuffers[i], NULL);
    }
    vkDestroyPipeline(vk.device, vk.graphics_pipeline, NULL);
    vkDestroyPipelineLayout(vk.device, vk.graphics_pipeline_layout, NULL);
    vkDestroyRenderPass(vk.device, vk.renderpass, NULL);
    for (u32 i = 0; i < vk.image_count; ++i) {
        vkDestroyImageView(vk.device, vk.swapchain_image_views[i], NULL);
    }
    vkDestroySwapchainKHR(vk.device, vk.swapchain, NULL);
}

static bool swapchain_reinit(void)
{
    LOGWARN("Recreating swapchain");
    swapchain_deinit();
    return swapchain_init();
}

static i64 find_memory_type(u32 type_filter, VkMemoryPropertyFlags properties)
{
    VkPhysicalDeviceMemoryProperties mem_props;
    vkGetPhysicalDeviceMemoryProperties(vk.pdevice, &mem_props);
    for (u32 i = 0; i < mem_props.memoryTypeCount; ++i) {
        if ((type_filter & (1 << i)) && (mem_props.memoryTypes[i].propertyFlags & properties) == properties) {
            return i;
        }
    }

    return -1;
}

static bool create_buffer(VkDeviceSize size, VkBufferUsageFlags usage, VkMemoryPropertyFlags properties, VkBuffer *buffer, VkDeviceMemory *buffer_memory)
{
    VKRETURN(vkCreateBuffer(vk.device, &(VkBufferCreateInfo) {
        .sType = VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO,
        .size = size,
        .usage = usage,
        .sharingMode = VK_SHARING_MODE_EXCLUSIVE
    }, NULL, buffer));

    VkMemoryRequirements mem_reqs;
    vkGetBufferMemoryRequirements(vk.device, *buffer, &mem_reqs);

    const i64 mem_type = find_memory_type(mem_reqs.memoryTypeBits, properties);
    VKASSERT(mem_type != -1, "failed to find suitable memory type");

    VKRETURN(vkAllocateMemory(vk.device, &(VkMemoryAllocateInfo) {
        .sType = VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO,
        .allocationSize = mem_reqs.size,
        .memoryTypeIndex = mem_type
    }, NULL, buffer_memory));
    VKRETURN(vkBindBufferMemory(vk.device, *buffer, *buffer_memory, 0));
    return true;
}

static bool copy_buffer(VkBuffer src, VkBuffer dst, VkDeviceSize size)
{
    VkCommandBuffer commandbuffer;
    VKRETURN(vkAllocateCommandBuffers(vk.device, &(VkCommandBufferAllocateInfo) {
        .sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO,
        .level = VK_COMMAND_BUFFER_LEVEL_PRIMARY,
        .commandPool = vk.commandpool,
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
    VKRETURN(vkQueueSubmit(vk.graphics_queue, 1, &(VkSubmitInfo) {
        .sType = VK_STRUCTURE_TYPE_SUBMIT_INFO,
        .commandBufferCount = 1,
        .pCommandBuffers = &commandbuffer
    }, VK_NULL_HANDLE));
    VKRETURN(vkQueueWaitIdle(vk.graphics_queue));
    vkFreeCommandBuffers(vk.device, vk.commandpool, 1, &commandbuffer);
    return true;
}

bool renderer_init(struct WindowToRendererInfo *win)
{
    // create our reference to vulkan
    VKRETURN(vkCreateInstance(&(VkInstanceCreateInfo) {
        .sType = VK_STRUCTURE_TYPE_INSTANCE_CREATE_INFO,
        .pApplicationInfo = &(VkApplicationInfo) {
            .sType = VK_STRUCTURE_TYPE_APPLICATION_INFO,
            .apiVersion = VK_API_VERSION_1_0
        },
        .enabledExtensionCount = 2,
        .ppEnabledExtensionNames = (const char *[]) {VK_KHR_SURFACE_EXTENSION_NAME, CURRENT_SURFACE_EXTENSION_NAME},
        .enabledLayerCount = 1,
        .ppEnabledLayerNames = (const char *[]) {"VK_LAYER_KHRONOS_validation"}
    }, NULL, &vk.instance));

    // create a platform-specific surface which we can draw on
    {
        #if defined(VK_USE_PLATFORM_WIN32_KHR)
        VKRETURN(vkCreateWin32SurfaceKHR(vk.instance, &(VkWin32SurfaceCreateInfoKHR) {
            .sType = VK_STRUCTURE_TYPE_WIN32_SURFACE_CREATE_INFO_KHR,
            .hinstance = win->inst,
            .hwnd = win->hwnd
        }, NULL, &vk.surface));
        #elif defined(VK_USE_PLATFORM_XLIB_KHR)
        VKRETURN(vkCreateXlibSurfaceKHR(vk.instance, &(VkXlibSurfaceCreateInfoKHR) {
            .sType = VK_STRUCTURE_TYPE_XLIB_SURFACE_CREATE_INFO_KHR,
            .dpy = win->dpy,
            .window = win->win
        }, NULL, &vk.surface));
        #endif /* VK_USE_PLATFORM_WIN32_KHR, VK_USE_PLATFORM_XLIB_KHR */
    }

    // select physical device
    {
        u32 count = 1;
        VKRETURN(vkEnumeratePhysicalDevices(vk.instance, &count, &vk.pdevice));
        VKASSERT(count == 1, "failed to find physical device supporting vulkan");
    }

    // find indices of queue families
    {
        vk.graphics_family_index = -1;
        vk.present_family_index = -1;
        #define MAX_QUEUE_FAMILIES 16
        u32 count = MAX_QUEUE_FAMILIES;
        VkQueueFamilyProperties props[MAX_QUEUE_FAMILIES];
        #undef MAX_QUEUE_FAMILIES
        vkGetPhysicalDeviceQueueFamilyProperties(vk.pdevice, &count, props);
        for (u32 i = 0; i < count; ++i) {
            if (props[i].queueFlags & VK_QUEUE_GRAPHICS_BIT) {
                vk.graphics_family_index = i;
            }

            VkBool32 present = VK_FALSE;
            VKRETURN(vkGetPhysicalDeviceSurfaceSupportKHR(vk.pdevice, i, vk.surface, &present));
            if (present) {
                vk.present_family_index = i;
            }
        }
        VKASSERT(vk.graphics_family_index != -1, "failed to find graphics compatible queue");
        VKASSERT(vk.present_family_index != -1, "failed to find present compatible queue");

        // create a mathematical set of queue family indices
        const i64 all_indices[MAX_UNIQUE_QUEUE_FAMILIES] = {vk.graphics_family_index, vk.present_family_index};
        for (usize i = 0; i < MAX_UNIQUE_QUEUE_FAMILIES; ++i) {
            bool new = true;
            for (usize j = 0; j < vk.unique_family_indices_count; ++j) {
                if (vk.unique_family_indices[j] == all_indices[i]) {
                    new = false;
                    break;
                }
            }
            if (new) {
                vk.unique_family_indices[vk.unique_family_indices_count++] = all_indices[i];
            }
        }
    }

    // create vulkan representation of gpu
    {
        VkDeviceQueueCreateInfo infos[MAX_UNIQUE_QUEUE_FAMILIES];
        for (usize i = 0; i < vk.unique_family_indices_count; ++i) {
            infos[i] = (VkDeviceQueueCreateInfo) {
                .sType = VK_STRUCTURE_TYPE_DEVICE_QUEUE_CREATE_INFO,
                .queueCount = 1,
                .queueFamilyIndex = vk.unique_family_indices[i],
                .pQueuePriorities = (float []) {1.0f}
            };
        }
        VKRETURN(vkCreateDevice(vk.pdevice, &(VkDeviceCreateInfo) {
            .sType = VK_STRUCTURE_TYPE_DEVICE_CREATE_INFO,
            .queueCreateInfoCount = vk.unique_family_indices_count,
            .pQueueCreateInfos = infos,
            .enabledExtensionCount = 1,
            .ppEnabledExtensionNames = (const char *[]) {VK_KHR_SWAPCHAIN_EXTENSION_NAME},
            .pEnabledFeatures = &(VkPhysicalDeviceFeatures) {0}
        }, NULL, &vk.device));
    }

    if (!swapchain_init())
        return false;
    LOGTEMP("Image Count: %d", vk.image_count);

    // get handles to queues inside our device
    {
        vkGetDeviceQueue(vk.device, vk.graphics_family_index, 0, &vk.graphics_queue);
        vkGetDeviceQueue(vk.device, vk.present_family_index, 0, &vk.present_queue);
    }

    // command pool
    {
        VKRETURN(vkCreateCommandPool(vk.device, &(VkCommandPoolCreateInfo) {
            .sType = VK_STRUCTURE_TYPE_COMMAND_POOL_CREATE_INFO,
            .flags = VK_COMMAND_POOL_CREATE_RESET_COMMAND_BUFFER_BIT,
            .queueFamilyIndex = vk.graphics_family_index
        }, NULL, &vk.commandpool));
    }

    // vertex buffer
    {
        const VkDeviceSize size = sizeof(vertices);
        VkBuffer staging_buffer;
        VkDeviceMemory staging_buffer_memory;

        VKASSERT(create_buffer(size, VK_BUFFER_USAGE_TRANSFER_SRC_BIT, VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT|VK_MEMORY_PROPERTY_HOST_COHERENT_BIT, &staging_buffer, &staging_buffer_memory), "failed to create staging buffer");
        void *data;
        VKRETURN(vkMapMemory(vk.device, staging_buffer_memory, 0, size, 0, &data));
            memcpy(data, vertices, size);
        vkUnmapMemory(vk.device, staging_buffer_memory);

        VKASSERT(create_buffer(size, VK_BUFFER_USAGE_TRANSFER_DST_BIT|VK_BUFFER_USAGE_VERTEX_BUFFER_BIT, VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT, &vk.vertex_buffer, &vk.vertex_buffer_memory), "failed to create vertex buffer");

        VKASSERT(copy_buffer(staging_buffer, vk.vertex_buffer, size), "failed to copy staging buffer to vertex buffer");

        vkDestroyBuffer(vk.device, staging_buffer, NULL);
        vkFreeMemory(vk.device, staging_buffer_memory, NULL);
    }

    // command buffers
    {
        VKRETURN(vkAllocateCommandBuffers(vk.device, &(VkCommandBufferAllocateInfo) {
            .sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO,
            .commandPool = vk.commandpool,
            .level = VK_COMMAND_BUFFER_LEVEL_PRIMARY,
            .commandBufferCount = MAX_FRAMES_IN_FLIGHT
        }, vk.commandbuffers));
    }

    // sync objects
    {
        for (usize i = 0; i < MAX_FRAMES_IN_FLIGHT; ++i) {
            VKRETURN(vkCreateSemaphore(vk.device, &(VkSemaphoreCreateInfo) {
                .sType = VK_STRUCTURE_TYPE_SEMAPHORE_CREATE_INFO
            }, NULL, vk.image_available_semaphores+i));

            VKRETURN(vkCreateSemaphore(vk.device, &(VkSemaphoreCreateInfo) {
                .sType = VK_STRUCTURE_TYPE_SEMAPHORE_CREATE_INFO
            }, NULL, vk.render_complete_semaphores+i));

            VKRETURN(vkCreateFence(vk.device, &(VkFenceCreateInfo) {
                .sType = VK_STRUCTURE_TYPE_FENCE_CREATE_INFO,
                .flags = VK_FENCE_CREATE_SIGNALED_BIT
            }, NULL, vk.in_flight_fences+i));
        }
    }
    return true;
}

static bool record_command_buffer(VkCommandBuffer buffer, u32 image_index)
{
    VKRETURN(vkBeginCommandBuffer(buffer, &(VkCommandBufferBeginInfo) {
        .sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO
    }));
    vkCmdBeginRenderPass(buffer, &(VkRenderPassBeginInfo) {
        .sType = VK_STRUCTURE_TYPE_RENDER_PASS_BEGIN_INFO,
        .renderPass = vk.renderpass,
        .framebuffer = vk.swapchain_framebuffers[image_index],
        .renderArea.extent = vk.extent,
        .clearValueCount = 1,
        .pClearValues = &(VkClearValue) {.color={.float32={0.2f, 0.3f, 0.8f, 1.0f}}}
    }, VK_SUBPASS_CONTENTS_INLINE);
    vkCmdBindPipeline(buffer, VK_PIPELINE_BIND_POINT_GRAPHICS, vk.graphics_pipeline);
    vkCmdBindVertexBuffers(buffer, 0, 1, &vk.vertex_buffer, (VkDeviceSize []) {0});
    vkCmdDraw(buffer, LENGTH(vertices), 1, 0, 0);
    vkCmdEndRenderPass(buffer);
    VKRETURN(vkEndCommandBuffer(buffer));
    return true;
}

void renderer_update(void)
{
    persist u32 current_frame = 0;
    vkWaitForFences(vk.device, 1, vk.in_flight_fences+current_frame, VK_TRUE, UINT64_MAX);

    u32 image_index;
    VkResult result = vkAcquireNextImageKHR(vk.device, vk.swapchain, UINT64_MAX, vk.image_available_semaphores[current_frame],  VK_NULL_HANDLE, &image_index);
    if (result == VK_ERROR_OUT_OF_DATE_KHR) {
        if (!swapchain_reinit()) {
            LOGERROR("failed to recreate swapchain, exiting forcefully...");
            exit(EXIT_FAILURE);
        }
        return;
    }
    if (result != VK_SUCCESS && result != VK_SUBOPTIMAL_KHR) {
        LOGERROR("failed to acquire swapchain image, exiting forcefully...");
        exit(EXIT_FAILURE);
    }

    vkResetFences(vk.device, 1, vk.in_flight_fences+current_frame);

    vkResetCommandBuffer(vk.commandbuffers[current_frame], 0);
    if (!record_command_buffer(vk.commandbuffers[current_frame], image_index)) {
        LOGERROR("failed to record command buffer");
    }

    // TODO: check for submission errors
    vkQueueSubmit(vk.graphics_queue, 1, &(VkSubmitInfo) {
        .sType = VK_STRUCTURE_TYPE_SUBMIT_INFO,
        .waitSemaphoreCount = 1,
        .pWaitSemaphores = vk.image_available_semaphores+current_frame,
        .pWaitDstStageMask = (VkPipelineStageFlags []) {VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT},
        .commandBufferCount = 1,
        .pCommandBuffers = vk.commandbuffers+current_frame,
        .signalSemaphoreCount = 1,
        .pSignalSemaphores = vk.render_complete_semaphores+current_frame
    }, vk.in_flight_fences[current_frame]);

    result = vkQueuePresentKHR(vk.present_queue, &(VkPresentInfoKHR) {
        .sType = VK_STRUCTURE_TYPE_PRESENT_INFO_KHR,
        .waitSemaphoreCount = 1,
        .pWaitSemaphores = vk.render_complete_semaphores+current_frame,
        .swapchainCount = 1,
        .pSwapchains = &vk.swapchain,
        .pImageIndices = &image_index
    });

    if (result == VK_ERROR_OUT_OF_DATE_KHR || result == VK_SUBOPTIMAL_KHR) {
        swapchain_reinit();
    } else if (result != VK_SUCCESS) {
        LOGERROR("failed to present swapchain image, exiting forcefully...");
        exit(EXIT_FAILURE);
    }

    current_frame = (current_frame + 1) % MAX_FRAMES_IN_FLIGHT;
}

void renderer_deinit(void)
{
    swapchain_deinit();
    for (usize i = 0; i < MAX_FRAMES_IN_FLIGHT; ++i) {
        vkDestroyFence(vk.device, vk.in_flight_fences[i], NULL);
        vkDestroySemaphore(vk.device, vk.render_complete_semaphores[i], NULL);
        vkDestroySemaphore(vk.device, vk.image_available_semaphores[i], NULL);
    }
    vkDestroyBuffer(vk.device, vk.vertex_buffer, NULL);
    vkFreeMemory(vk.device, vk.vertex_buffer_memory, NULL);
    vkDestroyCommandPool(vk.device, vk.commandpool, NULL);

    vkDestroyDevice(vk.device, NULL);
    vkDestroySurfaceKHR(vk.instance, vk.surface, NULL);
    vkDestroyInstance(vk.instance, NULL);
}

#endif /* INCLUDE_SRC */

#endif /* VK_RENDERER */
