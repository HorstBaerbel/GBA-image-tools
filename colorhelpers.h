// some color / color map utility functions used by the tools
#pragma once

#include <cstdint>
#include <vector>
#include <string>
#include <Magick++.h>

using namespace Magick;

/// @brief Get color map from ImageMagick Image
std::vector<Color> getColorMap(const Image &img);

/// @brief set color map in an ImageMagick Image
void setColorMap(Image &img, const std::vector<Color> &colorMap);

/// @brief Convert ImageMagick color to string
std::string asHex(const Color &color);

/// @brief Add color to color map and shift all other colors to the back by one
std::vector<Color> addColorAtIndex0(const std::vector<Color> &colorMap, const Color &color0);

/// @brief Convert ImageMagick Colors to BGR555 colors for GBA
std::vector<std::vector<uint16_t>> convertToBGR555(const std::vector<std::vector<Color>> &colors);

/// @brief Convert ImageMagick Colors to BGR555 colors for GBA
std::vector<uint16_t> convertToBGR555(const std::vector<Color> &colors);

/// @brief Convert a ImageMagick Color to a BGR555 color for GBA
uint16_t colorToBGR555(const Color &color);

/// @brief Build an image with the RGB555 color map the GBA uses
Image buildColorMapRGB555();

/// @brief Interleave all palette colors: P0C0, P1C0, P0C1, P1C1...
std::vector<Color> interleave(const std::vector<std::vector<Color>> &palettes);

/// @brief Calculate distance between colors
/// See: https://stackoverflow.com/a/40950076 and https://www.compuphase.com/cmetric.htm
float distance(const Color &a, const Color &b);

/// @brief Reorder palette colors to minimize preceived color distance
/// See: https://stackoverflow.com/a/40950076 and https://www.compuphase.com/cmetric.htm
std::vector<uint8_t> minimizeColorDistance(const std::vector<Color> &colors);

/// @brief Swap colors in palette according to index table
std::vector<Color> swapColors(const std::vector<Color> &colors, const std::vector<uint8_t> &newIndices);
