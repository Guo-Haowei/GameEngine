#include "file_access.h"

#include "core/string/string_utils.h"

namespace my {

FileAccess::CreateFunc FileAccess::s_create_func[ACCESS_MAX];

auto FileAccess::create(AccessType p_access_type) -> std::shared_ptr<FileAccess> {
    DEV_ASSERT_INDEX(p_access_type, ACCESS_MAX);

    auto ret = s_create_func[p_access_type]();
    ret->setAccessType(p_access_type);
    return std::shared_ptr<FileAccess>(ret);
}

auto FileAccess::createForPath(const std::string& p_path) -> std::shared_ptr<FileAccess> {
    if (p_path.starts_with("@res://")) {
        return create(ACCESS_RESOURCE);
    }

    if (p_path.starts_with("@user://")) {
        return create(ACCESS_USERDATA);
    }

    return create(ACCESS_FILESYSTEM);
}

auto FileAccess::open(const std::string& p_path, ModeFlags p_mode_flags)
    -> std::expected<std::shared_ptr<FileAccess>, Error<ErrorCode>> {
    auto file_access = createForPath(p_path);

    ErrorCode err = file_access->openInternal(file_access->fixPath(p_path), p_mode_flags);
    if (err != OK) {
        return VCT_ERROR(err, "error code: {}", std::to_underlying(err));
    }

    return file_access;
}

std::string FileAccess::fixPath(std::string_view p_path) {
    std::string fixed_path{ p_path };
    switch (m_access_type) {
        case ACCESS_RESOURCE: {
            if (p_path.starts_with("@res://")) {
                StringUtils::ReplaceFirst(fixed_path, "@res:/", ROOT_FOLDER "resources");
                return fixed_path;
            }
        } break;
        case ACCESS_USERDATA: {
            if (p_path.starts_with("@user://")) {
                StringUtils::ReplaceFirst(fixed_path, "@user:/", ROOT_FOLDER "user");
                return fixed_path;
            }
        } break;
        default:
            break;
    }
    return fixed_path;
}

}  // namespace my
