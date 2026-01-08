#pragma once

#include <vector>
#include <cstdint>

namespace TextureUtils
{
    /**
     * Creates a checkerboard pattern texture data for debugging
     *
     * @param size The width/height of the square texture (should be power of 2 for best results)
     * @param checkSize The size of each checker square in pixels (default: size/8)
     * @return Vector containing RGBA texture data (black and white checkerboard)
     *
     * Usage example:
     *   auto data = TextureUtils::createCheckerboard(256);
     *   texture->setData(data.data(), 256, 256, TextureFormat::RGBA);
     */
    std::vector<uint8_t> createCheckerboard(uint32_t size, uint32_t checkSize = 0);

    /**
     * Creates a solid color texture
     *
     * @param width Texture width
     * @param height Texture height
     * @param r Red component (0-255)
     * @param g Green component (0-255)
     * @param b Blue component (0-255)
     * @param a Alpha component (0-255)
     * @return Vector containing RGBA texture data
     */
    std::vector<uint8_t> createSolidColor(uint32_t width, uint32_t height,
                                          uint8_t r, uint8_t g, uint8_t b, uint8_t a = 255);

    /**
     * Creates a gradient texture (horizontal)
     *
     * @param width Texture width
     * @param height Texture height
     * @return Vector containing RGBA texture data
     */
    std::vector<uint8_t> createGradient(uint32_t width, uint32_t height);
}
