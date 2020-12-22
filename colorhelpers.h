// some color / color map utility functions used by the tools
#pragma once

#include <cstdint>
#include <vector>
#include <Magick++.h>

using namespace Magick;

/// @brief Get color map from ImageMagick Image.
std::vector<Color> getColorMap(const Image &img);

/// @brief set color map in an ImageMagick Image.
void setColorMap(Image &img, const std::vector<Color> &colorMap);

/// @brief Add color to color map and shift all other colors to the back by one
std::vector<Color> addColor0ToColorMap(const std::vector<Color> colorMap, const Color &color0);

/// @brief Convert ImageMagick Colors to RGB555 colors for GBA.
std::vector<uint16_t> convertToBGR555(const std::vector<Color> &colors);

/// @brief Convert a ImageMagick Color to a RGB555 color for GBA.
uint16_t colorToBGR555(const Color &color);
