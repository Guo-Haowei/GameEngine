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

    virtual void close() = 0;
    virtual bool isOpen() const = 0;
    virtual size_t getLength() const = 0;

    virtual bool readBuffer(void* p_data, size_t p_size) const = 0;
    virtual bool writeBuffer(const void* p_data, size_t p_size) = 0;

    AccessType getAccessType() const { return m_access_type; }

    static auto create(AccessType p_access_type) -> std::shared_ptr<FileAccess>;
    static auto createForPath(const std::string& p_path) -> std::shared_ptr<FileAccess>;

    static auto open(const std::string& p_path, ModeFlags p_mode_flags) -> std::expected<std::shared_ptr<FileAccess>, Error<ErrorCode>>;
    static auto open(const FilePath& p_path, ModeFlags p_mode_flags) -> std::expected<std::shared_ptr<FileAccess>, Error<ErrorCode>> {
        return open(p_path.string(), p_mode_flags);
    }

    template<typename T>
    static void makeDefault(AccessType p_access_type) {
        s_create_func[p_access_type] = createBuiltin<T>;
    }

protected:
    FileAccess() = default;

    virtual ErrorCode openInternal(std::string_view p_path, ModeFlags p_mode_flags) = 0;
    virtual void setAccessType(AccessType p_access_type) { m_access_type = p_access_type; }
    virtual std::string fixPath(std::string_view p_path);

    AccessType m_access_type = ACCESS_MAX;

    static CreateFunc s_create_func[ACCESS_MAX];

private:
    template<typename T>
    static FileAccess* createBuiltin() {
        return new T;
    }
};

}  // namespace my
