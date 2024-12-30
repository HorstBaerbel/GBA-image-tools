#include "colorhelpers.h"

#include <algorithm>

namespace ColorHelpers
{

    auto addColorAtIndex0(const std::vector<Color::XRGB8888> &colorMap, const Color::XRGB8888 &color0) -> std::vector<Color::XRGB8888>
    {
        std::vector<Color::XRGB8888> tempMap(colorMap.size() + 1, color0);
        std::copy(colorMap.cbegin(), colorMap.cend(), std::next(tempMap.begin()));
        return tempMap;
    }

    auto swapColors(const std::vector<Color::XRGB8888> &colors, const std::vector<uint8_t> &newIndices) -> std::vector<Color::XRGB8888>
    {
        std::vector<Color::XRGB8888> result;
        for (uint32_t i = 0; i < colors.size(); i++)
        {
            result.push_back(colors.at(newIndices[i]));
        }
        return result;
    }

    auto buildColorMapRGB555() -> std::vector<Color::XRGB8888>
    {
        std::vector<Color::XRGB8888> result;
        for (uint32_t r = 0; r < 32; ++r)
        {
            const auto cr = static_cast<uint8_t>((255 * r) / 31);
            for (uint32_t g = 0; g < 32; ++g)
            {
                const auto cg = static_cast<uint8_t>((255 * g) / 31);
                for (uint32_t b = 0; b < 32; ++b)
                {
                    const auto cb = static_cast<uint8_t>((255 * b) / 31);
                    result.emplace_back(Color::XRGB8888(cr, cg, cb));
                }
            }
        }
        return result;
    }

    auto buildColorMapRGB565() -> std::vector<Color::XRGB8888>
    {
        std::vector<Color::XRGB8888> result;
        for (uint32_t r = 0; r < 32; ++r)
        {
            const auto cr = static_cast<uint8_t>((255 * r) / 31);
            for (uint32_t g = 0; g < 64; ++g)
            {
                const auto cg = static_cast<uint8_t>((255 * g) / 63);
                for (uint32_t b = 0; b < 32; ++b)
                {
                    const auto cb = static_cast<uint8_t>((255 * b) / 31);
                    result.emplace_back(Color::XRGB8888(cr, cg, cb));
                }
            }
        }
        return result;
    }

    auto buildColorMapFor(Color::Format format) -> std::vector<Color::XRGB8888>
    {
        if (format == Color::Format::XRGB1555)
        {
            return buildColorMapRGB555();
        }
        else if (format == Color::Format::RGB565)
        {
            return buildColorMapRGB565();
        }
        THROW(std::runtime_error, "Unsupported color format");
    }
}
