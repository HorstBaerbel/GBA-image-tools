#pragma once

#include "colorformat.h"
#include "xrgb8888.h"

#include <cstdint>
#include <limits>
#include <vector>

namespace ColorHelpers
{

    /// @brief Add color to color map and shift all other colors towards the end by one
    auto addColorAtIndex0(const std::vector<Color::XRGB8888> &colorMap, const Color::XRGB8888 &color0) -> std::vector<Color::XRGB8888>;

    /// @brief Swap colors in list according to index table. The assignment is result[i] = colors[newIndices[i]];
    auto swapColors(const std::vector<Color::XRGB8888> &colors, const std::vector<uint8_t> &newIndices) -> std::vector<Color::XRGB8888>;

    /// @brief Build a color map with all colors in the RGB555 color space the GBA uses
    auto buildColorMapRGB555() -> std::vector<Color::XRGB8888>;

    /// @brief Build a color map with all colors in the RGB565 color space the NDS or DXT use
    auto buildColorMapRGB565() -> std::vector<Color::XRGB8888>;

    /// @brief Build a color map for color format color space. Only works for XRGB1555 and RGB565. All other formats will throw
    auto buildColorMapFor(Color::Format format) -> std::vector<Color::XRGB8888>;

    /// @brief Find color closest to input color in list of colors
    template <typename T, typename R>
    auto getClosestColor(T color, const std::vector<R> &colors) -> T
    {
        R colorR;
        if constexpr (std::is_same<T, R>())
        {
            colorR = color;
        }
        else
        {
            colorR = convertTo<R>(color);
        }
        // Note: We don't use min_element here due to double distance function evaluations
        // Improvement: Use distance query acceleration structure
        auto colorIt = colors.cbegin();
        auto closestColor = *colorIt;
        float closestDistance = std::numeric_limits<float>::max();
        while (colorIt != colors.cend())
        {
            auto colorDistance = R::mse(*colorIt, colorR);
            if (closestDistance > colorDistance)
            {
                closestDistance = colorDistance;
                closestColor = *colorIt;
            }
            ++colorIt;
        }
        if constexpr (std::is_same<T, R>())
        {
            return closestColor;
        }
        else
        {
            return convertTo<T>(closestColor);
        }
    }

    /// @brief Convert pixels to XRGB8888
    /// @param pixels Pixel data
    /// @param pixelFormat Pixel format
    /// @return XRGB8888 pixel data
    auto toXRGB8888(const std::vector<uint8_t> &pixels, Color::Format pixelFormat) -> std::vector<Color::XRGB8888>;

    /// @brief Convert pixels to XRGB8888
    /// @param pixels Pixel data
    /// @param pixelFormat Pixel format
    /// @param colorMap Color map
    /// @param colorMapFormat Color map format
    /// @return XRGB8888 pixel data
    auto toXRGB8888(const std::vector<uint8_t> &pixels, Color::Format pixelFormat, const std::vector<Color::XRGB8888> &colorMap) -> std::vector<Color::XRGB8888>;
}
