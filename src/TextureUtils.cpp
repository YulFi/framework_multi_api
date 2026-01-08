#include "TextureUtils.h"
#include "Logger.h"
#include <algorithm>

namespace TextureUtils
{

std::vector<uint8_t> createCheckerboard(uint32_t size, uint32_t checkSize)
{
    // Default to 8x8 checkers if checkSize not specified
    if (checkSize == 0)
    {
        checkSize = size / 8;
        if (checkSize == 0) checkSize = 1;
    }

    // RGBA format: 4 bytes per pixel
    std::vector<uint8_t> data(size * size * 4);

    for (uint32_t y = 0; y < size; y++)
    {
        for (uint32_t x = 0; x < size; x++)
        {
            // Determine which checker we're in
            uint32_t checkerX = x / checkSize;
            uint32_t checkerY = y / checkSize;

            // Alternate between black and white
            bool isWhite = (checkerX + checkerY) % 2 == 0;
            uint8_t color = isWhite ? 255 : 0;

            // Calculate pixel index (RGBA format)
            uint32_t index = (y * size + x) * 4;

            // Set RGBA values
            data[index + 0] = color;  // R
            data[index + 1] = color;  // G
            data[index + 2] = color;  // B
            data[index + 3] = 255;    // A (fully opaque)
        }
    }

    LOG_INFO("[TextureUtils] Created {}x{} checkerboard texture (check size: {})",
             size, size, checkSize);

    return data;
}

std::vector<uint8_t> createSolidColor(uint32_t width, uint32_t height,
                                      uint8_t r, uint8_t g, uint8_t b, uint8_t a)
{
    std::vector<uint8_t> data(width * height * 4);

    for (uint32_t i = 0; i < width * height; i++)
    {
        uint32_t index = i * 4;
        data[index + 0] = r;
        data[index + 1] = g;
        data[index + 2] = b;
        data[index + 3] = a;
    }

    LOG_DEBUG("[TextureUtils] Created {}x{} solid color texture (R:{}, G:{}, B:{}, A:{})",
              width, height, r, g, b, a);

    return data;
}

std::vector<uint8_t> createGradient(uint32_t width, uint32_t height)
{
    std::vector<uint8_t> data(width * height * 4);

    for (uint32_t y = 0; y < height; y++)
    {
        for (uint32_t x = 0; x < width; x++)
        {
            uint32_t index = (y * width + x) * 4;

            // Horizontal gradient from black to white
            uint8_t color = static_cast<uint8_t>((x * 255) / width);

            data[index + 0] = color;  // R
            data[index + 1] = color;  // G
            data[index + 2] = color;  // B
            data[index + 3] = 255;    // A
        }
    }

    LOG_DEBUG("[TextureUtils] Created {}x{} gradient texture", width, height);

    return data;
}

} // namespace TextureUtils
