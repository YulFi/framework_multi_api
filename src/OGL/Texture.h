#pragma once

#include "../RenderAPI/ITexture.h"
#include <glad/glad.h>

namespace OGL
{
    class Texture : public ITexture
    {
    public:
        Texture();
        ~Texture() override;

        Texture(const Texture&) = delete;
        Texture& operator=(const Texture&) = delete;

        Texture(Texture&& other) noexcept;
        Texture& operator=(Texture&& other) noexcept;

        void bind(uint32_t slot = 0) override;
        void unbind() override;

        void setData(const void* data, uint32_t width, uint32_t height, TextureFormat format) override;
        void updateData(const void* data, uint32_t xOffset, uint32_t yOffset,
                       uint32_t width, uint32_t height) override;

        void setFilter(TextureFilter minFilter, TextureFilter magFilter) override;
        void setWrap(TextureWrap wrapS, TextureWrap wrapT) override;
        void generateMipmaps() override;

        uint32_t getWidth() const override { return m_width; }
        uint32_t getHeight() const override { return m_height; }
        TextureFormat getFormat() const override { return m_format; }

        GLuint getID() const { return m_textureID; }

    private:
        GLenum convertTextureFormat(TextureFormat format) const;
        GLenum convertInternalFormat(TextureFormat format) const;
        GLenum convertFilter(TextureFilter filter) const;
        GLenum convertWrap(TextureWrap wrap) const;

        GLuint m_textureID;
        uint32_t m_width;
        uint32_t m_height;
        TextureFormat m_format;
    };
}
