#pragma once

namespace my {

// @TODO: refactor concepts
template<typename T>
concept TriviallyCopyable = std::is_trivially_copyable_v<T> && (!std::is_pointer_v<T>);

class FileAccess {
public:
    using CreateFunc = FileAccess* (*)(void);
    using GetUserFolderFunc = std::function<const char*()>;
    using GetResourceFolderFunc = std::function<const char*()>;

    enum AccessType : uint8_t {
        ACCESS_FILESYSTEM,
        ACCESS_RESOURCE,
        ACCESS_USERDATA,
        ACCESS_MAX,
    };

    enum ModeFlags : uint8_t {
        NONE = BIT(0),
        READ = BIT(1),
        WRITE = BIT(2),
        CREATE = BIT(3),
        TRUNCATE = BIT(4),
        EXCLUSIVE = BIT(5),

        READ_WRITE = READ | WRITE,
    };

    virtual ~FileAccess() = default;

    virtual void Close() = 0;
    virtual bool IsOpen() const = 0;
    virtual size_t GetLength() const = 0;

    virtual size_t ReadBuffer(void* p_data, size_t p_size) const = 0;
    virtual size_t WriteBuffer(const void* p_data, size_t p_size) = 0;
    // @TODO: refactor the error codes
    virtual long Tell() = 0;
    virtual int Seek(long p_offset) = 0;

    template<TriviallyCopyable T>
    size_t Read(T& p_data) {
        return ReadBuffer(&p_data, sizeof(T));
    }

    template<TriviallyCopyable T>
    size_t Write(const T& p_data) {
        return WriteBuffer(&p_data, sizeof(T));
    }

    template<int N>
    size_t Read(char (&p_data)[N]) {
        return ReadBuffer(p_data, N);
    }

    template<int N>
    size_t Write(const char (&p_data)[N]) {
        return WriteBuffer(p_data, N);
    }

    AccessType GetAccessType() const { return m_accessType; }
    ModeFlags GetOpenMode() const { return m_openMode; }

    static void SetUserFolderCallback(GetUserFolderFunc p_user_func) {
        s_getUserFolderFunc = p_user_func;
    }

    static void SetResFolderCallback(GetResourceFolderFunc p_resource_func) {
        s_getResourceFolderFunc = p_resource_func;
    }

    static auto Create(AccessType p_access_type) -> std::shared_ptr<FileAccess>;
    static auto CreateForPath(std::string_view p_path) -> std::shared_ptr<FileAccess>;

    static auto Open(std::string_view p_path, ModeFlags p_mode_flags) -> Result<std::shared_ptr<FileAccess>>;

    template<typename T>
    static void MakeDefault(AccessType p_access_type) {
        s_createFuncs[p_access_type] = CreateBuiltin<T>;
    }

    static std::string FixPath(AccessType p_access_type, std::string_view p_path);

protected:
    FileAccess() = default;

    virtual auto OpenInternal(std::string_view p_path, ModeFlags p_mode_flags) -> Result<void> = 0;
    virtual void SetAccessType(AccessType p_access_type) { m_accessType = p_access_type; }

    AccessType m_accessType = ACCESS_MAX;
    ModeFlags m_openMode = NONE;

    static CreateFunc s_createFuncs[ACCESS_MAX];

private:
    template<typename T>
    static FileAccess* CreateBuiltin() {
        return new T;
    }

    static GetUserFolderFunc s_getUserFolderFunc;
    static GetResourceFolderFunc s_getResourceFolderFunc;
};

DEFINE_ENUM_BITWISE_OPERATIONS(FileAccess::ModeFlags);

}  // namespace my
