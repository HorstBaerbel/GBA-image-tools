#pragma once

#include "xrgb888.h"

#include <cstdint>
#include <vector>

/// @brief Add color to color map and shift all other colors towards the end by one
auto addColorAtIndex0(const std::vector<Color::XRGB888> &colorMap, const Color::XRGB888 &color0) -> std::vector<Color::XRGB888>;

/// @brief Build an image with the RGB555 color space the GBA uses
auto buildColorMapRGB555() -> std::vector<Color::XRGB888>;

/// @brief Build an image with the RGB565 color space the NDS or DXT
auto buildColorMapRGB565() -> std::vector<Color::XRGB888>;

/// @brief Swap colors in palette according to index table
auto swapColors(const std::vector<Color::XRGB888> &colors, const std::vector<uint8_t> &newIndices) -> std::vector<Color::XRGB888>;
