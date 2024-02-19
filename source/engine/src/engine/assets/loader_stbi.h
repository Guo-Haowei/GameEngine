#pragma once
#include "assets/image.h"
#include "assets/loader.h"

namespace my {

class LoaderSTBIBase : public Loader<Image> {
public:
    using STBILoadFunc = void* (*)(uint8_t const* buffer, int len, int* x, int* y, int* comp, int req_comp);

    using Base = Loader<Image>;
    LoaderSTBIBase(const std::string& path) : Base{ path } {}

    bool load_impl(Image* data, bool is_float, STBILoadFunc p_func);
};

class LoaderSTBI8 : public LoaderSTBIBase {

public:
    LoaderSTBI8(const std::string& path) : LoaderSTBIBase{ path } {}

    bool load(Image* data) override;

    static std::shared_ptr<Base> create(const std::string& path) {
        return std::make_shared<LoaderSTBI8>(path);
    }
};

class LoaderSTBI32 : public LoaderSTBIBase {
    using Base = Loader<Image>;

public:
    LoaderSTBI32(const std::string& path) : LoaderSTBIBase{ path } {}

    bool load(Image* data) override;

    static std::shared_ptr<Base> create(const std::string& path) {
        return std::make_shared<LoaderSTBI32>(path);
    }
};

}  // namespace my
