#pragma once
#include "file_path.h"

namespace my {

class FileAccess {
public:
    using CreateFunc = FileAccess* (*)(void);

    enum AccessType {
        ACCESS_FILESYSTEM,
        ACCESS_RESOURCE,
        ACCESS_USERDATA,
        ACCESS_MAX,
    };

    enum ModeFlags {
        NONE = 0,
        READ = 1,
        WRITE = 2,
    };

    virtual ~FileAccess() = default;

    virtual void Close() = 0;
    virtual bool IsOpen() const = 0;
    virtual size_t GetLength() const = 0;

    virtual bool ReadBuffer(void* p_data, size_t p_size) const = 0;
    virtual bool WriteBuffer(const void* p_data, size_t p_size) = 0;

    AccessType GetAccessType() const { return m_accessType; }

    static auto Create(AccessType p_access_type) -> std::shared_ptr<FileAccess>;
    static auto CreateForPath(std::string_view p_path) -> std::shared_ptr<FileAccess>;

    static auto Open(std::string_view p_path, ModeFlags p_mode_flags) -> Result<std::shared_ptr<FileAccess>>;
    static auto Open(const FilePath& p_path, ModeFlags p_mode_flags) -> Result<std::shared_ptr<FileAccess>> {
        return Open(p_path.StringView(), p_mode_flags);
    }

    template<typename T>
    static void MakeDefault(AccessType p_access_type) {
        s_createFuncs[p_access_type] = CreateBuiltin<T>;
    }

protected:
    FileAccess() = default;

    virtual auto OpenInternal(std::string_view p_path, ModeFlags p_mode_flags) -> Result<void> = 0;
    virtual void SetAccessType(AccessType p_access_type) { m_accessType = p_access_type; }
    virtual std::string FixPath(std::string_view p_path);

    AccessType m_accessType = ACCESS_MAX;

    static CreateFunc s_createFuncs[ACCESS_MAX];

private:
    template<typename T>
    static FileAccess* CreateBuiltin() {
        return new T;
    }
};

}  // namespace my
