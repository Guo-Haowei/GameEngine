#include "vulkan_helpers.h"

namespace my {

VkResult ReportVkErrorIfFailed(VkResult p_result,
                               std::string_view p_function,
                               std::string_view p_file,
                               int p_line,
                               std::string_view p_error) {
    if (p_result != 0) {
        auto error_code = std::format("error code: {}({})", ToString(p_result), std::to_underlying(p_result));
        ReportErrorImpl(p_function, p_file, p_line, p_error, error_code);
    }
    return p_result;
}

const char* ToString(VkResult p_result) {
    switch (p_result) {
#define VK_RESULT(ENUM)    \
    case ::VkResult::ENUM: \
        return #ENUM;
        VK_RESULT_LIST
#undef VK_RESULT
        default:
            return "VK_ERROR_UNKNOWN";
    }
}

const char* ToString(VkDebugReportObjectTypeEXT p_type) {
    switch (p_type) {
#define VK_DEBUG_REPORT_OBJECT_TYPE(ENUM)                                  \
    case ::VkDebugReportObjectTypeEXT::VK_DEBUG_REPORT_OBJECT_TYPE_##ENUM: \
        return "VK_DEBUG_REPORT_OBJECT_TYPE_" #ENUM;
        VK_DEBUG_REPORT_OBJECT_TYPE_LIST
#undef VK_DEBUG_REPORT_OBJECT_TYPE
        default:
            return "VK_DEBUG_REPORT_OBJECT_TYPE_UNKNOWN";
    }
}

}  // namespace my
