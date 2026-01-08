#pragma once

#include <cstddef>
#include <cstdint>

enum class TextureFormat
{
    RGB,
    RGBA,
    Red,
    RG,
    Depth
};

enum class TextureFilter
{
    Nearest,
    Linear
};

enum class TextureWrap
{
    Repeat,
    ClampToEdge,
    ClampToBorder,
    MirroredRepeat
};

class ITexture
{
public:
    virtual ~ITexture() = default;

    virtual void bind(uint32_t slot = 0) = 0;
    virtual void unbind() = 0;

    // Create texture from raw data
    virtual void setData(const void* data, uint32_t width, uint32_t height, TextureFormat format) = 0;

    // Update texture data (partial or full)
    virtual void updateData(const void* data, uint32_t xOffset, uint32_t yOffset,
                           uint32_t width, uint32_t height) = 0;

    // Set texture parameters
    virtual void setFilter(TextureFilter minFilter, TextureFilter magFilter) = 0;
    virtual void setWrap(TextureWrap wrapS, TextureWrap wrapT) = 0;

    // Generate mipmaps
    virtual void generateMipmaps() = 0;

    // Getters
    virtual uint32_t getWidth() const = 0;
    virtual uint32_t getHeight() const = 0;
    virtual TextureFormat getFormat() const = 0;
};
