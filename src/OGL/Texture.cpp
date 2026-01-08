#include "Texture.h"
#include "../Logger.h"
#include <stdexcept>

namespace OGL
{

Texture::Texture()
    : m_textureID(0)
    , m_width(0)
    , m_height(0)
    , m_format(TextureFormat::RGBA)
{
    glGenTextures(1, &m_textureID);
    if (m_textureID == 0)
    {
        throw std::runtime_error("Failed to create OpenGL texture");
    }
    LOG_DEBUG("[OpenGL] Texture created (ID: {})", m_textureID);
}

Texture::~Texture()
{
    if (m_textureID != 0)
    {
        glDeleteTextures(1, &m_textureID);
        LOG_DEBUG("[OpenGL] Texture destroyed (ID: {})", m_textureID);
        m_textureID = 0;
    }
}

Texture::Texture(Texture&& other) noexcept
    : m_textureID(other.m_textureID)
    , m_width(other.m_width)
    , m_height(other.m_height)
    , m_format(other.m_format)
{
    other.m_textureID = 0;
    other.m_width = 0;
    other.m_height = 0;
}

Texture& Texture::operator=(Texture&& other) noexcept
{
    if (this != &other)
    {
        if (m_textureID != 0)
        {
            glDeleteTextures(1, &m_textureID);
        }

        m_textureID = other.m_textureID;
        m_width = other.m_width;
        m_height = other.m_height;
        m_format = other.m_format;

        other.m_textureID = 0;
        other.m_width = 0;
        other.m_height = 0;
    }
    return *this;
}

void Texture::bind(uint32_t slot)
{
    glActiveTexture(GL_TEXTURE0 + slot);
    glBindTexture(GL_TEXTURE_2D, m_textureID);
    LOG_DEBUG("[OpenGL] Texture bound to slot {} (ID: {})", slot, m_textureID);
}

void Texture::unbind()
{
    glBindTexture(GL_TEXTURE_2D, 0);
}

void Texture::setData(const void* data, uint32_t width, uint32_t height, TextureFormat format)
{
    m_width = width;
    m_height = height;
    m_format = format;

    bind(0);

    GLenum glFormat = convertTextureFormat(format);
    GLenum internalFormat = convertInternalFormat(format);

    glTexImage2D(GL_TEXTURE_2D, 0, internalFormat, width, height, 0, glFormat, GL_UNSIGNED_BYTE, data);

    // Check for errors
    GLenum error = glGetError();
    if (error != GL_NO_ERROR)
    {
        LOG_ERROR("[OpenGL] Error setting texture data: 0x{:X}", error);
    }

    // Set default filtering and wrapping
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT);

    LOG_INFO("[OpenGL] Texture data set ({}x{}, format: {}, ID: {})", width, height, static_cast<int>(format), m_textureID);
}

void Texture::updateData(const void* data, uint32_t xOffset, uint32_t yOffset,
                        uint32_t width, uint32_t height)
{
    if (m_textureID == 0)
    {
        LOG_ERROR("[OpenGL] Cannot update texture data - texture not initialized");
        return;
    }

    bind(0);
    GLenum glFormat = convertTextureFormat(m_format);
    glTexSubImage2D(GL_TEXTURE_2D, 0, xOffset, yOffset, width, height, glFormat, GL_UNSIGNED_BYTE, data);
}

void Texture::setFilter(TextureFilter minFilter, TextureFilter magFilter)
{
    bind(0);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, convertFilter(minFilter));
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, convertFilter(magFilter));
}

void Texture::setWrap(TextureWrap wrapS, TextureWrap wrapT)
{
    bind(0);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, convertWrap(wrapS));
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, convertWrap(wrapT));
}

void Texture::generateMipmaps()
{
    bind(0);
    glGenerateMipmap(GL_TEXTURE_2D);
    LOG_DEBUG("[OpenGL] Mipmaps generated for texture (ID: {})", m_textureID);
}

GLenum Texture::convertTextureFormat(TextureFormat format) const
{
    switch (format)
    {
        case TextureFormat::RGB:   return GL_RGB;
        case TextureFormat::RGBA:  return GL_RGBA;
        case TextureFormat::Red:   return GL_RED;
        case TextureFormat::RG:    return GL_RG;
        case TextureFormat::Depth: return GL_DEPTH_COMPONENT;
        default:                   return GL_RGBA;
    }
}

GLenum Texture::convertInternalFormat(TextureFormat format) const
{
    switch (format)
    {
        case TextureFormat::RGB:   return GL_RGB8;
        case TextureFormat::RGBA:  return GL_RGBA8;
        case TextureFormat::Red:   return GL_R8;
        case TextureFormat::RG:    return GL_RG8;
        case TextureFormat::Depth: return GL_DEPTH_COMPONENT24;
        default:                   return GL_RGBA8;
    }
}

GLenum Texture::convertFilter(TextureFilter filter) const
{
    switch (filter)
    {
        case TextureFilter::Nearest: return GL_NEAREST;
        case TextureFilter::Linear:  return GL_LINEAR;
        default:                     return GL_LINEAR;
    }
}

GLenum Texture::convertWrap(TextureWrap wrap) const
{
    switch (wrap)
    {
        case TextureWrap::Repeat:         return GL_REPEAT;
        case TextureWrap::ClampToEdge:    return GL_CLAMP_TO_EDGE;
        case TextureWrap::ClampToBorder:  return GL_CLAMP_TO_BORDER;
        case TextureWrap::MirroredRepeat: return GL_MIRRORED_REPEAT;
        default:                          return GL_REPEAT;
    }
}

} // namespace OGL
