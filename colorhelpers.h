// some color / color map utility functions used by the tools
#pragma once

#include <cstdint>
#include <vector>
#include <string>
#include <Magick++.h>

/// @brief Convert ImageMagick color to string
std::string asHex(const Magick::Color &color);

/// @brief Add color to color map and shift all other colors towards the end by one
std::vector<Magick::Color> addColorAtIndex0(const std::vector<Magick::Color> &colorMap, const Magick::Color &color0);

/// @brief Convert ImageMagick Colors to BGR555 colors for GBA
std::vector<std::vector<uint16_t>> convertToBGR555(const std::vector<std::vector<Magick::Color>> &colors);

/// @brief Convert ImageMagick Colors to BGR555 colors for GBA
std::vector<uint16_t> convertToBGR555(const std::vector<Magick::Color> &colors);

/// @brief Convert a ImageMagick Color to a BGR555 color for GBA
uint16_t colorToBGR555(const Magick::Color &color);

/// @brief Convert ImageMagick Colors to BGR565 colors
std::vector<uint16_t> convertToBGR565(const std::vector<Magick::Color> &colors);

/// @brief Convert a ImageMagick Color to a BGR565 color
uint16_t colorToBGR565(const Magick::Color &color);

/// @brief Convert ImageMagick Colors to BGR888 colors
std::vector<uint8_t> convertToBGR888(const std::vector<Magick::Color> &colors);

/// @brief Build an image with the RGB555 color map the GBA uses
Magick::Image buildColorMapRGB555();

/// @brief Interpolate between two RGB565 colors
uint16_t lerpRGB565(uint16_t a, uint16_t b, double t);

/// @brief Calculate perceived distance between colors
/// See: https://stackoverflow.com/a/40950076 and https://www.compuphase.com/cmetric.htm
/// @return Returns a value in [0,2.8284)
float distance(const Magick::Color &a, const Magick::Color &b);

/// @brief Calculate square of perceived distance between colors (see above)
/// See: https://stackoverflow.com/a/40950076 and https://www.compuphase.com/cmetric.htm
/// @return Returns a value in [0,8]
float distanceSqr(const Magick::Color &a, const Magick::Color &b);

/// @brief Reorder palette colors to minimize preceived color distance
/// See: https://stackoverflow.com/a/40950076 and https://www.compuphase.com/cmetric.htm
std::vector<uint8_t> minimizeColorDistance(const std::vector<Magick::Color> &colors);

/// @brief Swap colors in palette according to index table
std::vector<Magick::Color> swapColors(const std::vector<Magick::Color> &colors, const std::vector<uint8_t> &newIndices);

/// @brief Build table that stores the squared percieved distance of one RGB565 color to another RGB565 color as [0,255]
/// @note This table is ~4GB in size!
std::vector<std::vector<uint8_t>> RGB565DistanceSqrTable();
