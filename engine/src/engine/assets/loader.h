#pragma once
#include "engine/core/io/file_path.h"

namespace my {

struct IAsset;

class LoaderBase {
public:
    LoaderBase(const std::string& path);

    virtual ~LoaderBase() = default;

    const std::string& GetError() { return m_error; }

protected:
    std::string m_fileName;
    std::string m_filePath;
    std::string m_basePath;
    std::string m_extension;
    std::string m_error;
};

template<typename T>
class Loader : public LoaderBase {
    using CreateLoaderFunc = std::shared_ptr<Loader<T>> (*)(const std::string& path);

public:
    Loader(const std::string& p_path) : LoaderBase(p_path) {}

    virtual bool Load(T* p_data) = 0;

    static std::shared_ptr<Loader<T>> Create(const FilePath& p_path) {
        std::string ext = p_path.ExtensionCStr();
        auto it = s_loaderCreator.find(ext);
        if (it == s_loaderCreator.end()) {
            return nullptr;
        }
        return it->second(p_path.String());
    }

    static bool RegisterLoader(const std::string& p_extension, CreateLoaderFunc p_func) {
        DEV_ASSERT(!p_extension.empty());
        DEV_ASSERT(p_func);
        auto it = s_loaderCreator.find(p_extension);
        if (it != s_loaderCreator.end()) {
            LOG_WARN("Already exists a loader for p_extension '{}'", p_extension);
            it->second = p_func;
        } else {
            s_loaderCreator[p_extension] = p_func;
        }
        return true;
    }

protected:
    inline static std::map<std::string, CreateLoaderFunc> s_loaderCreator;
};

}  // namespace my
