#pragma once
#include "vulkan/vulkan.h"

#define VK_OP(EXPR) ReportVkErrorIfFailed((EXPR), __FUNCTION__, __FILE__, __LINE__, #EXPR)
#define VK_CHECK_ERROR(EXPR, CODE)                                                   \
    do {                                                                             \
        if (VkResult _result = VK_OP(EXPR); _result != VK_SUCCESS) [[unlikely]] {    \
            return HBN_ERROR(CODE, "VkResult: {}, " #EXPR, ::my::ToString(_result)); \
        }                                                                            \
    } while (0)

namespace my {

VkResult ReportVkErrorIfFailed(VkResult p_result,
                               std::string_view p_function,
                               std::string_view p_file,
                               int p_line,
                               std::string_view p_error);

const char* ToString(VkResult p_result);
const char* ToString(VkDebugReportObjectTypeEXT p_type);

}  // namespace my

#define VK_RESULT_LIST                                               \
    VK_RESULT(VK_SUCCESS)                                            \
    VK_RESULT(VK_NOT_READY)                                          \
    VK_RESULT(VK_TIMEOUT)                                            \
    VK_RESULT(VK_EVENT_SET)                                          \
    VK_RESULT(VK_EVENT_RESET)                                        \
    VK_RESULT(VK_INCOMPLETE)                                         \
    VK_RESULT(VK_ERROR_OUT_OF_HOST_MEMORY)                           \
    VK_RESULT(VK_ERROR_OUT_OF_DEVICE_MEMORY)                         \
    VK_RESULT(VK_ERROR_INITIALIZATION_FAILED)                        \
    VK_RESULT(VK_ERROR_DEVICE_LOST)                                  \
    VK_RESULT(VK_ERROR_MEMORY_MAP_FAILED)                            \
    VK_RESULT(VK_ERROR_LAYER_NOT_PRESENT)                            \
    VK_RESULT(VK_ERROR_EXTENSION_NOT_PRESENT)                        \
    VK_RESULT(VK_ERROR_FEATURE_NOT_PRESENT)                          \
    VK_RESULT(VK_ERROR_INCOMPATIBLE_DRIVER)                          \
    VK_RESULT(VK_ERROR_TOO_MANY_OBJECTS)                             \
    VK_RESULT(VK_ERROR_FORMAT_NOT_SUPPORTED)                         \
    VK_RESULT(VK_ERROR_FRAGMENTED_POOL)                              \
    VK_RESULT(VK_ERROR_UNKNOWN)                                      \
    VK_RESULT(VK_ERROR_OUT_OF_POOL_MEMORY)                           \
    VK_RESULT(VK_ERROR_INVALID_EXTERNAL_HANDLE)                      \
    VK_RESULT(VK_ERROR_FRAGMENTATION)                                \
    VK_RESULT(VK_ERROR_INVALID_OPAQUE_CAPTURE_ADDRESS)               \
    VK_RESULT(VK_PIPELINE_COMPILE_REQUIRED)                          \
    VK_RESULT(VK_ERROR_SURFACE_LOST_KHR)                             \
    VK_RESULT(VK_ERROR_NATIVE_WINDOW_IN_USE_KHR)                     \
    VK_RESULT(VK_SUBOPTIMAL_KHR)                                     \
    VK_RESULT(VK_ERROR_OUT_OF_DATE_KHR)                              \
    VK_RESULT(VK_ERROR_INCOMPATIBLE_DISPLAY_KHR)                     \
    VK_RESULT(VK_ERROR_VALIDATION_FAILED_EXT)                        \
    VK_RESULT(VK_ERROR_INVALID_SHADER_NV)                            \
    VK_RESULT(VK_ERROR_IMAGE_USAGE_NOT_SUPPORTED_KHR)                \
    VK_RESULT(VK_ERROR_VIDEO_PICTURE_LAYOUT_NOT_SUPPORTED_KHR)       \
    VK_RESULT(VK_ERROR_VIDEO_PROFILE_OPERATION_NOT_SUPPORTED_KHR)    \
    VK_RESULT(VK_ERROR_VIDEO_PROFILE_FORMAT_NOT_SUPPORTED_KHR)       \
    VK_RESULT(VK_ERROR_VIDEO_PROFILE_CODEC_NOT_SUPPORTED_KHR)        \
    VK_RESULT(VK_ERROR_VIDEO_STD_VERSION_NOT_SUPPORTED_KHR)          \
    VK_RESULT(VK_ERROR_INVALID_DRM_FORMAT_MODIFIER_PLANE_LAYOUT_EXT) \
    VK_RESULT(VK_ERROR_NOT_PERMITTED_KHR)                            \
    VK_RESULT(VK_ERROR_FULL_SCREEN_EXCLUSIVE_MODE_LOST_EXT)          \
    VK_RESULT(VK_THREAD_IDLE_KHR)                                    \
    VK_RESULT(VK_THREAD_DONE_KHR)                                    \
    VK_RESULT(VK_OPERATION_DEFERRED_KHR)                             \
    VK_RESULT(VK_OPERATION_NOT_DEFERRED_KHR)                         \
    VK_RESULT(VK_ERROR_COMPRESSION_EXHAUSTED_EXT)                    \
    VK_RESULT(VK_ERROR_INCOMPATIBLE_SHADER_BINARY_EXT)

#define VK_DEBUG_REPORT_OBJECT_TYPE_LIST                        \
    VK_DEBUG_REPORT_OBJECT_TYPE(UNKNOWN_EXT)                    \
    VK_DEBUG_REPORT_OBJECT_TYPE(INSTANCE_EXT)                   \
    VK_DEBUG_REPORT_OBJECT_TYPE(PHYSICAL_DEVICE_EXT)            \
    VK_DEBUG_REPORT_OBJECT_TYPE(DEVICE_EXT)                     \
    VK_DEBUG_REPORT_OBJECT_TYPE(QUEUE_EXT)                      \
    VK_DEBUG_REPORT_OBJECT_TYPE(SEMAPHORE_EXT)                  \
    VK_DEBUG_REPORT_OBJECT_TYPE(COMMAND_BUFFER_EXT)             \
    VK_DEBUG_REPORT_OBJECT_TYPE(FENCE_EXT)                      \
    VK_DEBUG_REPORT_OBJECT_TYPE(DEVICE_MEMORY_EXT)              \
    VK_DEBUG_REPORT_OBJECT_TYPE(BUFFER_EXT)                     \
    VK_DEBUG_REPORT_OBJECT_TYPE(IMAGE_EXT)                      \
    VK_DEBUG_REPORT_OBJECT_TYPE(EVENT_EXT)                      \
    VK_DEBUG_REPORT_OBJECT_TYPE(QUERY_POOL_EXT)                 \
    VK_DEBUG_REPORT_OBJECT_TYPE(BUFFER_VIEW_EXT)                \
    VK_DEBUG_REPORT_OBJECT_TYPE(IMAGE_VIEW_EXT)                 \
    VK_DEBUG_REPORT_OBJECT_TYPE(SHADER_MODULE_EXT)              \
    VK_DEBUG_REPORT_OBJECT_TYPE(PIPELINE_CACHE_EXT)             \
    VK_DEBUG_REPORT_OBJECT_TYPE(PIPELINE_LAYOUT_EXT)            \
    VK_DEBUG_REPORT_OBJECT_TYPE(RENDER_PASS_EXT)                \
    VK_DEBUG_REPORT_OBJECT_TYPE(PIPELINE_EXT)                   \
    VK_DEBUG_REPORT_OBJECT_TYPE(DESCRIPTOR_SET_LAYOUT_EXT)      \
    VK_DEBUG_REPORT_OBJECT_TYPE(SAMPLER_EXT)                    \
    VK_DEBUG_REPORT_OBJECT_TYPE(DESCRIPTOR_POOL_EXT)            \
    VK_DEBUG_REPORT_OBJECT_TYPE(DESCRIPTOR_SET_EXT)             \
    VK_DEBUG_REPORT_OBJECT_TYPE(FRAMEBUFFER_EXT)                \
    VK_DEBUG_REPORT_OBJECT_TYPE(COMMAND_POOL_EXT)               \
    VK_DEBUG_REPORT_OBJECT_TYPE(SURFACE_KHR_EXT)                \
    VK_DEBUG_REPORT_OBJECT_TYPE(SWAPCHAIN_KHR_EXT)              \
    VK_DEBUG_REPORT_OBJECT_TYPE(DEBUG_REPORT_CALLBACK_EXT_EXT)  \
    VK_DEBUG_REPORT_OBJECT_TYPE(DISPLAY_KHR_EXT)                \
    VK_DEBUG_REPORT_OBJECT_TYPE(DISPLAY_MODE_KHR_EXT)           \
    VK_DEBUG_REPORT_OBJECT_TYPE(VALIDATION_CACHE_EXT_EXT)       \
    VK_DEBUG_REPORT_OBJECT_TYPE(SAMPLER_YCBCR_CONVERSION_EXT)   \
    VK_DEBUG_REPORT_OBJECT_TYPE(DESCRIPTOR_UPDATE_TEMPLATE_EXT) \
    VK_DEBUG_REPORT_OBJECT_TYPE(CU_MODULE_NVX_EXT)              \
    VK_DEBUG_REPORT_OBJECT_TYPE(CU_FUNCTION_NVX_EXT)            \
    VK_DEBUG_REPORT_OBJECT_TYPE(ACCELERATION_STRUCTURE_KHR_EXT) \
    VK_DEBUG_REPORT_OBJECT_TYPE(ACCELERATION_STRUCTURE_NV_EXT)  \
    VK_DEBUG_REPORT_OBJECT_TYPE(BUFFER_COLLECTION_FUCHSIA_EXT)
