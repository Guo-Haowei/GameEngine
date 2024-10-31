#pragma once
#include "assets/image.h"
#include "assets/loader.h"

namespace my {

class LoaderSTBIBase : public Loader<Image> {
public:
    using STBILoadFunc = void* (*)(uint8_t const* p_buffer, int p_len, int* p_x, int* p_y, int* p_comp, int p_req_comp);

    using Base = Loader<Image>;
    LoaderSTBIBase(const std::string& p_path) : Base{ p_path } {}

    bool LoadImpl(Image* p_data, bool p_is_float, STBILoadFunc p_func);
};

class LoaderSTBI8 : public LoaderSTBIBase {

public:
    LoaderSTBI8(const std::string& p_path) : LoaderSTBIBase{ p_path } {}

    bool Load(Image* p_data) override;

    static std::shared_ptr<Base> Create(const std::string& p_path) {
        return std::make_shared<LoaderSTBI8>(p_path);
    }
};

class LoaderSTBI32 : public LoaderSTBIBase {
    using Base = Loader<Image>;

public:
    LoaderSTBI32(const std::string& p_path) : LoaderSTBIBase{ p_path } {}

    bool Load(Image* p_data) override;

    static std::shared_ptr<Base> Create(const std::string& p_path) {
        return std::make_shared<LoaderSTBI32>(p_path);
    }
};

}  // namespace my
