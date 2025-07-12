#include "vulkan_graphics_manager.h"

#include "engine/drivers/empty/empty_pipeline_state_manager.h"
#include "engine/drivers/glfw/glfw_display_manager.h"
#include "engine/runtime/application.h"
#include "engine/runtime/imgui_manager.h"
#include "vulkan_helpers.h"
///////
#include "imgui/backends/imgui_impl_glfw.h"
#include "imgui/backends/imgui_impl_vulkan.h"
#include "imgui/imgui.h"
#define GLFW_INCLUDE_NONE
#define GLFW_INCLUDE_VULKAN
#include <GLFW/glfw3.h>

namespace my {

// #define IMGUI_UNLIMITED_FRAME_RATE
#ifdef _DEBUG
#define IMGUI_VULKAN_DEBUG_REPORT
#endif

// @TODO: move all the stuff to class

// Data
static VkAllocationCallbacks* g_Allocator = NULL;
static VkInstance g_Instance = VK_NULL_HANDLE;
static VkPhysicalDevice g_PhysicalDevice = VK_NULL_HANDLE;
static VkDevice g_Device = VK_NULL_HANDLE;
static uint32_t g_QueueFamily = (uint32_t)-1;
static VkQueue g_Queue = VK_NULL_HANDLE;
static VkDebugReportCallbackEXT g_DebugReport = VK_NULL_HANDLE;
static VkPipelineCache g_PipelineCache = VK_NULL_HANDLE;
static VkDescriptorPool g_DescriptorPool = VK_NULL_HANDLE;

static ImGui_ImplVulkanH_Window g_MainWindowData;
static int g_MinImageCount = 2;

static void check_vk_result(VkResult err) {
    if (err == 0) {
        return;
    }
    __debugbreak();
    fprintf(stderr, "[vulkan] Error: VkResult = %d\n", err);
    if (err < 0) {
        abort();
    }
}

static VKAPI_ATTR VkBool32 VKAPI_CALL DebugReport(VkDebugReportFlagsEXT p_flags,
                                                  VkDebugReportObjectTypeEXT p_object_type,
                                                  uint64_t p_object,
                                                  size_t p_location,
                                                  int32_t p_message_code,
                                                  const char* p_layer_prefix,
                                                  const char* p_message,
                                                  void* p_user_data) {
    unused(p_message_code);
    unused(p_object);
    unused(p_location);
    unused(p_user_data);
    unused(p_layer_prefix);

    LogLevel level = LOG_LEVEL_VERBOSE;
    if (p_flags & VK_DEBUG_REPORT_ERROR_BIT_EXT) {
        level = LOG_LEVEL_ERROR;
    } else if (p_flags & VK_DEBUG_REPORT_WARNING_BIT_EXT) {
        level = LOG_LEVEL_WARN;
    }

    LogImpl(level, "[vulkan] Debug report from ObjectType: {}\nMessage: {}\n", ToString(p_object_type), p_message);

    return VK_FALSE;
}

auto VulkanGraphicsManager::CreateInstance() -> Result<void> {
    uint32_t extensions_count = 0;
    const char** extensions = glfwGetRequiredInstanceExtensions(&extensions_count);

    // Create Vulkan Instance
    VkInstanceCreateInfo create_info = {};
    create_info.sType = VK_STRUCTURE_TYPE_INSTANCE_CREATE_INFO;
    create_info.enabledExtensionCount = extensions_count;
    create_info.ppEnabledExtensionNames = extensions;

    if (!m_enableValidationLayer) {
        // create instance and return if no validation layer needed
        VK_CHECK_ERROR(vkCreateInstance(&create_info, g_Allocator, &g_Instance),
                       ErrorCode::ERR_CANT_CREATE);

        return Result<void>();
    }

    const char* layers[] = { "VK_LAYER_KHRONOS_validation" };
    create_info.enabledLayerCount = 1;
    create_info.ppEnabledLayerNames = layers;

    std::vector<const char*> extensions_ext;
    extensions_ext.resize(extensions_count + 1);

    memcpy(extensions_ext.data(), extensions, extensions_count * sizeof(const char*));
    extensions_ext[extensions_count] = "VK_EXT_debug_report";
    create_info.enabledExtensionCount = extensions_count + 1;
    create_info.ppEnabledExtensionNames = extensions_ext.data();

    VK_CHECK_ERROR(vkCreateInstance(&create_info, g_Allocator, &g_Instance),
                   ErrorCode::ERR_CANT_CREATE);

    // Get the function pointer (required for any extensions)
    auto vkCreateDebugReportCallbackEXT = (PFN_vkCreateDebugReportCallbackEXT)vkGetInstanceProcAddr(g_Instance, "vkCreateDebugReportCallbackEXT");
    if (vkCreateDebugReportCallbackEXT == nullptr) {
        return HBN_ERROR(ErrorCode::ERR_CANT_CREATE, "failed to query vkCreateDebugReportCallbackEXT");
    }

    // Setup the debug report callback
    VkDebugReportCallbackCreateInfoEXT debug_report_ci = {};
    debug_report_ci.sType = VK_STRUCTURE_TYPE_DEBUG_REPORT_CALLBACK_CREATE_INFO_EXT;
    debug_report_ci.flags = VK_DEBUG_REPORT_ERROR_BIT_EXT | VK_DEBUG_REPORT_WARNING_BIT_EXT | VK_DEBUG_REPORT_PERFORMANCE_WARNING_BIT_EXT;
    debug_report_ci.pfnCallback = DebugReport;
    debug_report_ci.pUserData = nullptr;
    VK_CHECK_ERROR(vkCreateDebugReportCallbackEXT(g_Instance, &debug_report_ci, g_Allocator, &g_DebugReport),
                   ErrorCode::ERR_CANT_CREATE);

    return Result<void>();
}

auto VulkanGraphicsManager::SelectHardware() -> Result<void> {
    uint32_t gpu_count;
    VK_CHECK_ERROR(vkEnumeratePhysicalDevices(g_Instance, &gpu_count, nullptr),
                   ErrorCode::ERR_CANT_CREATE);
    DEV_ASSERT(gpu_count > 0);

    std::vector<VkPhysicalDevice> gpus;
    gpus.resize(gpu_count);

    VK_CHECK_ERROR(vkEnumeratePhysicalDevices(g_Instance, &gpu_count, gpus.data()),
                   ErrorCode::ERR_CANT_CREATE);

    // If a number >1 of GPUs got reported, find discrete GPU if present, or use first one available. This covers
    // most common cases (multi-gpu/integrated+dedicated graphics). Handling more complicated setups (multiple
    // dedicated GPUs) is out of scope of this sample.
    uint32_t selected_index = 0;
    for (uint32_t i = 0; i < gpu_count; ++i) {
        VkPhysicalDeviceProperties properties;
        vkGetPhysicalDeviceProperties(gpus[i], &properties);
        if (properties.deviceType == VK_PHYSICAL_DEVICE_TYPE_DISCRETE_GPU) {
            selected_index = i;
            break;
        }
    }

    g_PhysicalDevice = gpus[selected_index];

    // Select graphics queue family
    uint32_t queue_count;
    vkGetPhysicalDeviceQueueFamilyProperties(g_PhysicalDevice, &queue_count, nullptr);
    std::vector<VkQueueFamilyProperties> queues;
    queues.resize(queue_count);
    vkGetPhysicalDeviceQueueFamilyProperties(g_PhysicalDevice, &queue_count, queues.data());

    for (uint32_t i = 0; i < queue_count; i++) {
        if (queues[i].queueFlags & VK_QUEUE_GRAPHICS_BIT) {
            g_QueueFamily = i;
            break;
        }
    }

    IM_ASSERT(g_QueueFamily != (uint32_t)-1);

    // Create Logical Device (with 1 queue)
    int device_extension_count = 1;
    const char* device_extensions[] = { "VK_KHR_swapchain" };
    const float queue_priority[] = { 1.0f };
    VkDeviceQueueCreateInfo queue_info[1] = {};
    queue_info[0].sType = VK_STRUCTURE_TYPE_DEVICE_QUEUE_CREATE_INFO;
    queue_info[0].queueFamilyIndex = g_QueueFamily;
    queue_info[0].queueCount = 1;
    queue_info[0].pQueuePriorities = queue_priority;
    VkDeviceCreateInfo create_info = {};
    create_info.sType = VK_STRUCTURE_TYPE_DEVICE_CREATE_INFO;
    create_info.queueCreateInfoCount = sizeof(queue_info) / sizeof(queue_info[0]);
    create_info.pQueueCreateInfos = queue_info;
    create_info.enabledExtensionCount = device_extension_count;
    create_info.ppEnabledExtensionNames = device_extensions;
    VK_CHECK_ERROR(vkCreateDevice(g_PhysicalDevice, &create_info, g_Allocator, &g_Device),
                   ErrorCode::ERR_CANT_CREATE);

    vkGetDeviceQueue(g_Device, g_QueueFamily, 0, &g_Queue);
    return Result<void>();
}

auto VulkanGraphicsManager::CreateDescriptorPool() -> Result<void> {
    VkDescriptorPoolSize pool_sizes[] = {
        { VK_DESCRIPTOR_TYPE_SAMPLER, 1000 },
        { VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, 1000 },
        { VK_DESCRIPTOR_TYPE_SAMPLED_IMAGE, 1000 },
        { VK_DESCRIPTOR_TYPE_STORAGE_IMAGE, 1000 },
        { VK_DESCRIPTOR_TYPE_UNIFORM_TEXEL_BUFFER, 1000 },
        { VK_DESCRIPTOR_TYPE_STORAGE_TEXEL_BUFFER, 1000 },
        { VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER, 1000 },
        { VK_DESCRIPTOR_TYPE_STORAGE_BUFFER, 1000 },
        { VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER_DYNAMIC, 1000 },
        { VK_DESCRIPTOR_TYPE_STORAGE_BUFFER_DYNAMIC, 1000 },
        { VK_DESCRIPTOR_TYPE_INPUT_ATTACHMENT, 1000 }
    };
    VkDescriptorPoolCreateInfo pool_info = {};
    pool_info.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_POOL_CREATE_INFO;
    pool_info.flags = VK_DESCRIPTOR_POOL_CREATE_FREE_DESCRIPTOR_SET_BIT;
    pool_info.maxSets = 1000 * IM_ARRAYSIZE(pool_sizes);
    pool_info.poolSizeCount = (uint32_t)IM_ARRAYSIZE(pool_sizes);
    pool_info.pPoolSizes = pool_sizes;

    VK_CHECK_ERROR(vkCreateDescriptorPool(g_Device, &pool_info, g_Allocator, &g_DescriptorPool),
                   ErrorCode::ERR_CANT_CREATE);

    return Result<void>();
}

// All the ImGui_ImplVulkanH_XXX structures/functions are optional helpers used by the demo.
// Your real engine/app may not use them.
void SetupVulkanWindow(ImGui_ImplVulkanH_Window* wd, VkSurfaceKHR surface, int width, int height) {
    wd->Surface = surface;

    // Check for WSI support
    VkBool32 res;
    vkGetPhysicalDeviceSurfaceSupportKHR(g_PhysicalDevice, g_QueueFamily, wd->Surface, &res);
    if (res != VK_TRUE) {
        fprintf(stderr, "Error no WSI support on physical device 0\n");
        exit(-1);
    }

    // Select Surface Format
    const VkFormat requestSurfaceImageFormat[] = { VK_FORMAT_B8G8R8A8_UNORM, VK_FORMAT_R8G8B8A8_UNORM, VK_FORMAT_B8G8R8_UNORM, VK_FORMAT_R8G8B8_UNORM };
    const VkColorSpaceKHR requestSurfaceColorSpace = VK_COLORSPACE_SRGB_NONLINEAR_KHR;
    wd->SurfaceFormat = ImGui_ImplVulkanH_SelectSurfaceFormat(g_PhysicalDevice, wd->Surface, requestSurfaceImageFormat, (size_t)IM_ARRAYSIZE(requestSurfaceImageFormat), requestSurfaceColorSpace);

    // Select Present Mode
#ifdef IMGUI_UNLIMITED_FRAME_RATE
    VkPresentModeKHR present_modes[] = { VK_PRESENT_MODE_MAILBOX_KHR, VK_PRESENT_MODE_IMMEDIATE_KHR, VK_PRESENT_MODE_FIFO_KHR };
#else
    VkPresentModeKHR present_modes[] = { VK_PRESENT_MODE_FIFO_KHR };
#endif
    wd->PresentMode = ImGui_ImplVulkanH_SelectPresentMode(g_PhysicalDevice, wd->Surface, &present_modes[0], IM_ARRAYSIZE(present_modes));
    // printf("[vulkan] Selected PresentMode = %d\n", wd->PresentMode);

    // Create SwapChain, RenderPass, Framebuffer, etc.
    IM_ASSERT(g_MinImageCount >= 2);
    ImGui_ImplVulkanH_CreateOrResizeWindow(g_Instance, g_PhysicalDevice, g_Device, wd, g_QueueFamily, g_Allocator, width, height, g_MinImageCount);
}

void CleanupVulkan() {
    vkDestroyDescriptorPool(g_Device, g_DescriptorPool, g_Allocator);

#ifdef IMGUI_VULKAN_DEBUG_REPORT
    // Remove the debug report callback
    auto vkDestroyDebugReportCallbackEXT = (PFN_vkDestroyDebugReportCallbackEXT)vkGetInstanceProcAddr(g_Instance, "vkDestroyDebugReportCallbackEXT");
    vkDestroyDebugReportCallbackEXT(g_Instance, g_DebugReport, g_Allocator);
#endif  // IMGUI_VULKAN_DEBUG_REPORT

    vkDestroyDevice(g_Device, g_Allocator);
    vkDestroyInstance(g_Instance, g_Allocator);
}

void CleanupVulkanWindow() {
    ImGui_ImplVulkanH_DestroyWindow(g_Instance, g_Device, &g_MainWindowData, g_Allocator);
}

static void FrameRender(ImGui_ImplVulkanH_Window* wd, ImDrawData* draw_data) {
    VkResult err;

    VkSemaphore image_acquired_semaphore = wd->FrameSemaphores[wd->SemaphoreIndex].ImageAcquiredSemaphore;
    VkSemaphore render_complete_semaphore = wd->FrameSemaphores[wd->SemaphoreIndex].RenderCompleteSemaphore;
    err = vkAcquireNextImageKHR(g_Device, wd->Swapchain, UINT64_MAX, image_acquired_semaphore, VK_NULL_HANDLE, &wd->FrameIndex);
    DEV_ASSERT(err != VK_ERROR_OUT_OF_DATE_KHR && err != VK_SUBOPTIMAL_KHR);
    check_vk_result(err);

    ImGui_ImplVulkanH_Frame* fd = &wd->Frames[wd->FrameIndex];
    {
        err = vkWaitForFences(g_Device, 1, &fd->Fence, VK_TRUE, UINT64_MAX);  // wait indefinitely instead of periodically checking
        check_vk_result(err);

        err = vkResetFences(g_Device, 1, &fd->Fence);
        check_vk_result(err);
    }
    {
        err = vkResetCommandPool(g_Device, fd->CommandPool, 0);
        check_vk_result(err);
        VkCommandBufferBeginInfo info = {};
        info.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO;
        info.flags |= VK_COMMAND_BUFFER_USAGE_ONE_TIME_SUBMIT_BIT;
        err = vkBeginCommandBuffer(fd->CommandBuffer, &info);
        check_vk_result(err);
    }
    {
        VkRenderPassBeginInfo info = {};
        info.sType = VK_STRUCTURE_TYPE_RENDER_PASS_BEGIN_INFO;
        info.renderPass = wd->RenderPass;
        info.framebuffer = fd->Framebuffer;
        info.renderArea.extent.width = wd->Width;
        info.renderArea.extent.height = wd->Height;
        info.clearValueCount = 1;
        info.pClearValues = &wd->ClearValue;
        vkCmdBeginRenderPass(fd->CommandBuffer, &info, VK_SUBPASS_CONTENTS_INLINE);
    }

    // Record dear imgui primitives into command buffer
    ImGui_ImplVulkan_RenderDrawData(draw_data, fd->CommandBuffer);

    // Submit command buffer
    vkCmdEndRenderPass(fd->CommandBuffer);
    {
        VkPipelineStageFlags wait_stage = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT;
        VkSubmitInfo info = {};
        info.sType = VK_STRUCTURE_TYPE_SUBMIT_INFO;
        info.waitSemaphoreCount = 1;
        info.pWaitSemaphores = &image_acquired_semaphore;
        info.pWaitDstStageMask = &wait_stage;
        info.commandBufferCount = 1;
        info.pCommandBuffers = &fd->CommandBuffer;
        info.signalSemaphoreCount = 1;
        info.pSignalSemaphores = &render_complete_semaphore;

        err = vkEndCommandBuffer(fd->CommandBuffer);
        check_vk_result(err);
        err = vkQueueSubmit(g_Queue, 1, &info, fd->Fence);
        check_vk_result(err);
    }
}

static void FramePresent(ImGui_ImplVulkanH_Window* wd) {
    VkSemaphore render_complete_semaphore = wd->FrameSemaphores[wd->SemaphoreIndex].RenderCompleteSemaphore;
    VkPresentInfoKHR info = {};
    info.sType = VK_STRUCTURE_TYPE_PRESENT_INFO_KHR;
    info.waitSemaphoreCount = 1;
    info.pWaitSemaphores = &render_complete_semaphore;
    info.swapchainCount = 1;
    info.pSwapchains = &wd->Swapchain;
    info.pImageIndices = &wd->FrameIndex;
    VkResult err = vkQueuePresentKHR(g_Queue, &info);
    DEV_ASSERT(err != VK_ERROR_OUT_OF_DATE_KHR && err != VK_SUBOPTIMAL_KHR);
    check_vk_result(err);
    wd->SemaphoreIndex = (wd->SemaphoreIndex + 1) % wd->SemaphoreCount;  // Now we can use the next set of semaphores
}

//////////////////////////////////////////////////////////////////////////////////////////////////////

VulkanGraphicsManager::VulkanGraphicsManager()
    : GraphicsManager("VulkanGraphicsManager", Backend::VULKAN, NUM_FRAMES_IN_FLIGHT) {
    m_pipelineStateManager = std::make_shared<EmptyPipelineStateManager>();
}

auto VulkanGraphicsManager::InitializeInternal() -> Result<void> {
    auto display_manager = dynamic_cast<GlfwDisplayManager*>(m_app->GetDisplayServer());
    DEV_ASSERT(display_manager);
    if (!display_manager) {
        return HBN_ERROR(ErrorCode::ERR_INVALID_DATA, "display manager is nullptr");
    }

    m_window = display_manager->GetGlfwWindow();

    if (auto res = CreateInstance(); !res) {
        return HBN_ERROR(res.error());
    }
    if (auto res = SelectHardware(); !res) {
        return HBN_ERROR(res.error());
    }
    if (auto res = CreateDescriptorPool(); !res) {
        return HBN_ERROR(res.error());
    }

    VK_CHECK_ERROR(glfwCreateWindowSurface(g_Instance, m_window, g_Allocator, &m_surface),
                   ErrorCode::ERR_CANT_CREATE);

    auto imgui = m_app->GetImguiManager();
    if (imgui) {
        imgui->SetRenderCallbacks(
            [&]() {
                int w, h;
                glfwGetFramebufferSize(m_window, &w, &h);
                ImGui_ImplVulkanH_Window* wd = &g_MainWindowData;
                SetupVulkanWindow(wd, m_surface, w, h);
                // Setup Platform/Renderer backends
                ImGui_ImplVulkan_InitInfo init_info = {};
                init_info.Instance = g_Instance;
                init_info.PhysicalDevice = g_PhysicalDevice;
                init_info.Device = g_Device;
                init_info.QueueFamily = g_QueueFamily;
                init_info.Queue = g_Queue;
                init_info.PipelineCache = g_PipelineCache;
                init_info.DescriptorPool = g_DescriptorPool;
                init_info.RenderPass = wd->RenderPass;
                init_info.Subpass = 0;
                init_info.MinImageCount = g_MinImageCount;
                init_info.ImageCount = wd->ImageCount;
                init_info.MSAASamples = VK_SAMPLE_COUNT_1_BIT;
                init_info.Allocator = g_Allocator;
                init_info.CheckVkResultFn = check_vk_result;

                ImGui_ImplVulkan_Init(&init_info);

                ImGui_ImplVulkan_NewFrame();
            },
            []() {
                VkResult err = vkDeviceWaitIdle(g_Device);
                check_vk_result(err);
                ImGui_ImplVulkan_Shutdown();
            });
    }

    return Result<void>();
}

void VulkanGraphicsManager::FinalizeImpl() {
    if (g_Device) {
        VkResult err = vkDeviceWaitIdle(g_Device);
        check_vk_result(err);

        CleanupVulkanWindow();
        CleanupVulkan();
    }
}

void VulkanGraphicsManager::OnWindowResize(int p_width, int p_height) {
    if (p_width > 0 && p_height > 0) {
        ImGui_ImplVulkan_SetMinImageCount(g_MinImageCount);
        ImGui_ImplVulkanH_CreateOrResizeWindow(g_Instance, g_PhysicalDevice, g_Device, &g_MainWindowData, g_QueueFamily, g_Allocator, p_width, p_height, g_MinImageCount);
        g_MainWindowData.FrameIndex = 0;
    }
}

void VulkanGraphicsManager::Present() {
    ImGui_ImplVulkanH_Window* wd = &g_MainWindowData;

    Vector4f clear_color{ 0.3f, 0.4f, 0.3f, 1.0f };
    // Rendering
    wd->ClearValue.color.float32[0] = clear_color.x * clear_color.w;
    wd->ClearValue.color.float32[1] = clear_color.y * clear_color.w;
    wd->ClearValue.color.float32[2] = clear_color.z * clear_color.w;
    wd->ClearValue.color.float32[3] = clear_color.w;
    ImDrawData* main_draw_data = ImGui::GetDrawData();
    const bool main_is_minimized = (main_draw_data->DisplaySize.x <= 0.0f || main_draw_data->DisplaySize.y <= 0.0f);
    if (!main_is_minimized) {
        FrameRender(wd, main_draw_data);
    }

    ImGuiIO& io = ImGui::GetIO();
    // Update and Render additional Platform Windows
    if (io.ConfigFlags & ImGuiConfigFlags_ViewportsEnable) {
        ImGui::UpdatePlatformWindows();
        ImGui::RenderPlatformWindowsDefault();
    }

    // Present Main Platform Window
    if (!main_is_minimized) {
        FramePresent(wd);
    }
}

}  // namespace my
