// some color / color map utility functions used by the tools
#pragma once

#include <cstdint>
#include <vector>
#include <string>
#include <Magick++.h>

/// @brief Get color map from ImageMagick Image
std::vector<Magick::Color> getColorMap(const Magick::Image &img);

/// @brief set color map in an ImageMagick Image
void setColorMap(Magick::Image &img, const std::vector<Magick::Color> &colorMap);

/// @brief Convert ImageMagick color to string
std::string asHex(const Magick::Color &color);

/// @brief Add color to color map and shift all other colors to the back by one
std::vector<Magick::Color> addColorAtIndex0(const std::vector<Magick::Color> &colorMap, const Magick::Color &color0);

/// @brief Convert ImageMagick Colors to BGR555 colors for GBA
std::vector<std::vector<uint16_t>> convertToBGR555(const std::vector<std::vector<Magick::Color>> &colors);

/// @brief Convert ImageMagick Colors to BGR555 colors for GBA
std::vector<uint16_t> convertToBGR555(const std::vector<Magick::Color> &colors);

/// @brief Convert a ImageMagick Color to a BGR555 color for GBA
uint16_t colorToBGR555(const Magick::Color &color);

/// @brief Build an image with the RGB555 color map the GBA uses
Magick::Image buildColorMapRGB555();

/// @brief Interleave all palette colors: P0C0, P1C0, P0C1, P1C1...
std::vector<Magick::Color> interleave(const std::vector<std::vector<Magick::Color>> &palettes);

/// @brief Calculate distance between colors
/// See: https://stackoverflow.com/a/40950076 and https://www.compuphase.com/cmetric.htm
float distance(const Magick::Color &a, const Magick::Color &b);

/// @brief Reorder palette colors to minimize preceived color distance
/// See: https://stackoverflow.com/a/40950076 and https://www.compuphase.com/cmetric.htm
std::vector<uint8_t> minimizeColorDistance(const std::vector<Magick::Color> &colors);

/// @brief Swap colors in palette according to index table
std::vector<Magick::Color> swapColors(const std::vector<Magick::Color> &colors, const std::vector<uint8_t> &newIndices);
