#include "file_access.h"

#include "engine/core/string/string_utils.h"

namespace my {

FileAccess::CreateFunc FileAccess::s_createFuncs[ACCESS_MAX];
FileAccess::GetUserFolderFunc FileAccess::s_getUserFolderFunc;
FileAccess::GetResourceFolderFunc FileAccess::s_getResourceFolderFunc;

auto FileAccess::Create(AccessType p_access_type) -> std::shared_ptr<FileAccess> {
    DEV_ASSERT_INDEX(p_access_type, ACCESS_MAX);

    auto ret = s_createFuncs[p_access_type]();
    ret->SetAccessType(p_access_type);
    return std::shared_ptr<FileAccess>(ret);
}

auto FileAccess::CreateForPath(std::string_view p_path) -> std::shared_ptr<FileAccess> {
    if (p_path.starts_with("@res://")) {
        return Create(ACCESS_RESOURCE);
    }

    if (p_path.starts_with("@user://")) {
        return Create(ACCESS_USERDATA);
    }

    return Create(ACCESS_FILESYSTEM);
}

auto FileAccess::Open(std::string_view p_path, ModeFlags p_mode_flags) -> Result<std::shared_ptr<FileAccess>> {
    auto file_access = CreateForPath(p_path);

    if (auto res = file_access->OpenInternal(file_access->FixPath(p_path), p_mode_flags); !res) {
        return HBN_ERROR(res.error());
    }

    return file_access;
}

std::string FileAccess::FixPath(std::string_view p_path) {
    std::string fixed_path{ p_path };
    switch (m_accessType) {
        case ACCESS_RESOURCE: {
            if (p_path.starts_with("@res://")) {
                // @TODO: configure it somewhere
                StringUtils::ReplaceFirst(fixed_path, "@res:/", s_getResourceFolderFunc());
                return fixed_path;
            }
        } break;
        case ACCESS_USERDATA: {
            if (p_path.starts_with("@user://")) {
                StringUtils::ReplaceFirst(fixed_path, "@user:/", s_getUserFolderFunc());
                return fixed_path;
            }
        } break;
        default:
            break;
    }
    return fixed_path;
}

}  // namespace my
