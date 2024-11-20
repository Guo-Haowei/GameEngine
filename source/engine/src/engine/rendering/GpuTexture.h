#pragma once

// @TODO: refactor
struct Texture2DCreateInfo {
    uint32_t wrapS = 0;
    uint32_t wrapT = 0;
    uint32_t minFilter = 0;
    uint32_t magFilter = 0;
    uint32_t internalFormat;
    uint32_t format;
    uint32_t dataType;
    int width;
    int height;
};

struct Texture3DCreateInfo {
    uint32_t wrapS, wrapT, wrapR;
    uint32_t minFilter, magFilter;
    uint32_t format;
    int size;
    int mipLevel;
};

class OldTexture {
public:
    void create2DEmpty(const Texture2DCreateInfo& info);
    void create3DEmpty(const Texture3DCreateInfo& info);

    void destroy();
    void bindImageTexture(int i, int mipLevel = 0);
    void clear();
    void bind() const;
    void unbind() const;
    void genMipMap();

    inline uint32_t GetHandle() const { return mHandle; }

protected:
    uint32_t m_type;
    uint32_t m_format;
    uint32_t mHandle = 0;
};
