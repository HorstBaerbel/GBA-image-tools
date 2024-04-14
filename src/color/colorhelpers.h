#pragma once

#include "colorformat.h"
#include "xrgb8888.h"

#include <cstdint>
#include <vector>

namespace Color
{

    /// @brief Add color to color map and shift all other colors towards the end by one
    auto addColorAtIndex0(const std::vector<Color::XRGB8888> &colorMap, const Color::XRGB8888 &color0) -> std::vector<Color::XRGB8888>;

    /// @brief Build a color map with the RGB555 color space the GBA uses
    auto buildColorMapRGB555() -> std::vector<Color::XRGB8888>;

    /// @brief Build a color map with the RGB565 color space the NDS or DXT use
    auto buildColorMapRGB565() -> std::vector<Color::XRGB8888>;

    /// @brief Build a color map for color format color space. Only works for XRGB1555 and RGB565. All other formats will return empty color maps
    auto buildColorMapFor(Color::Format format) -> std::vector<Color::XRGB8888>;

    /// @brief Swap colors in palette according to index table
    auto swapColors(const std::vector<Color::XRGB8888> &colors, const std::vector<uint8_t> &newIndices) -> std::vector<Color::XRGB8888>;

}
