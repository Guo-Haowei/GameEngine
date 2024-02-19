#pragma once
#include "scene/scene.h"

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
    std::string m_error;
};

template<typename T>
class Loader : public LoaderBase {
    using CreateLoaderFunc = std::shared_ptr<Loader<T>> (*)(const std::string& path);

public:
    Loader(const std::string& path) : LoaderBase(path) {}

    virtual bool load(T* data) = 0;

    static std::shared_ptr<Loader<T>> create(const std::string& path) {
        std::filesystem::path ext{ path };
        ext = ext.extension();
        auto it = s_loader_creator.find(ext.string());
        if (it == s_loader_creator.end()) {
            return nullptr;
        }
        return it->second(path);
    }

    static bool register_loader(const std::string& extension, CreateLoaderFunc func) {
        DEV_ASSERT(!extension.empty());
        DEV_ASSERT(func);

        // @TODO: check override
        s_loader_creator[extension] = func;
        return true;
    }

protected:
    inline static std::map<std::string, CreateLoaderFunc> s_loader_creator;
};

}  // namespace my
