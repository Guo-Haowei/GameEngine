#pragma once
#include "GpuTexture.h"

#include "core/framework/asset_manager.h"
#include "drivers/opengl/opengl_prerequisites.h"

using std::string;

void OldTexture::bind() const { glBindTexture(m_type, mHandle); }

void OldTexture::unbind() const { glBindTexture(m_type, 0); }

void OldTexture::genMipMap() {
    // make sure texture is bond first
    glGenerateMipmap(m_type);
}

void OldTexture::create2DEmpty(const Texture2DCreateInfo& info) {
    m_type = GL_TEXTURE_2D;
    m_format = info.internalFormat;

    glGenTextures(1, &mHandle);

    bind();
    glTexImage2D(m_type, 0, info.internalFormat, info.width, info.height, 0, info.format, info.dataType, 0);

    if (info.wrapS) glTexParameteri(m_type, GL_TEXTURE_WRAP_S, info.wrapS);
    if (info.wrapT) glTexParameteri(m_type, GL_TEXTURE_WRAP_T, info.wrapT);
    if (info.minFilter) glTexParameteri(m_type, GL_TEXTURE_MIN_FILTER, info.minFilter);
    if (info.magFilter) glTexParameteri(m_type, GL_TEXTURE_MAG_FILTER, info.magFilter);

    unbind();
}

void OldTexture::create3DEmpty(const Texture3DCreateInfo& info) {
    m_type = GL_TEXTURE_3D;
    m_format = info.format;

    glGenTextures(1, &mHandle);
    bind();
    glTexParameteri(m_type, GL_TEXTURE_WRAP_S, info.wrapS);
    glTexParameteri(m_type, GL_TEXTURE_WRAP_T, info.wrapT);
    glTexParameteri(m_type, GL_TEXTURE_WRAP_R, info.wrapR);
    glTexParameteri(m_type, GL_TEXTURE_MIN_FILTER, info.minFilter);
    glTexParameteri(m_type, GL_TEXTURE_MAG_FILTER, info.magFilter);

    glTexStorage3D(m_type, info.mipLevel, m_format, info.size, info.size, info.size);
}

void OldTexture::destroy() {
    if (mHandle != 0) {
        glDeleteTextures(1, &mHandle);
    }
    mHandle = 0;
}

void OldTexture::bindImageTexture(int i, int mipLevel) {
    glBindImageTexture(i, mHandle, mipLevel, GL_TRUE, 0, GL_READ_WRITE, m_format);
}

void OldTexture::clear() {
    float clearColor[4] = { 0.0f, 0.0f, 0.0f, 0.0f };
    /// Hard code format for now
    glClearTexImage(mHandle, 0, GL_RGBA, GL_FLOAT, clearColor);
    // glClearTexImage(m_handle, 0, m_format, GL_FLOAT, clearColor);
}
