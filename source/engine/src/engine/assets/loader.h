#pragma once
#include "core/io/file_path.h"

namespace my {

class LoaderBase {
public:
    LoaderBase(const std::string& path);

    virtual ~LoaderBase() = default;

    const std::string& get_error() { return m_error; }

protected:
    std::string m_file_name;
    std::string m_file_path;
    std::string m_base_path;
    std::string m_extension;
    std::string m_error;
};

template<typename T>
class Loader : public LoaderBase {
    using CreateLoaderFunc = std::shared_ptr<Loader<T>> (*)(const std::string& path);

public:
    Loader(const std::string& p_path) : LoaderBase(p_path) {}

    virtual bool load(T* p_data) = 0;

    static std::shared_ptr<Loader<T>> create(const FilePath& p_path) {
        std::string ext = p_path.ExtensionCStr();
        auto it = s_loader_creator.find(ext);
        if (it == s_loader_creator.end()) {
            return nullptr;
        }
        return it->second(p_path.String());
    }

    static bool register_loader(const std::string& p_extension, CreateLoaderFunc p_func) {
        DEV_ASSERT(!p_extension.empty());
        DEV_ASSERT(p_func);
        auto it = s_loader_creator.find(p_extension);
        if (it != s_loader_creator.end()) {
            LOG_WARN("Already exists a loader for p_extension '{}'", p_extension);
            it->second = p_func;
        } else {
            s_loader_creator[p_extension] = p_func;
        }
        return true;
    }

protected:
    inline static std::map<std::string, CreateLoaderFunc> s_loader_creator;
};

}  // namespace my
