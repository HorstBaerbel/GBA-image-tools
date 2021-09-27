// some image utility functions used by the tools
#pragma once

#include <cstdint>
#include <vector>
#include <Magick++.h>

/// @brief Get ImageMagick image data (palette or truecolor) as raw data bytes
/// The format returned are Palette8 for paletted images and RGB888 for truecolor images
std::vector<uint8_t> getImageData(const Magick::Image &img);

/// @brief Get color map from ImageMagick Image
std::vector<Magick::Color> getColorMap(const Magick::Image &img);

/// @brief set color map in an ImageMagick Image
void setColorMap(Magick::Image &img, const std::vector<Magick::Color> &colorMap);

/// @brief Convert RGB888 to RGB555
std::vector<uint8_t> toRGB555(const std::vector<uint8_t> &imageData);

/// @brief Convert RGB888 to RGB565
std::vector<uint8_t> toRGB565(const std::vector<uint8_t> &imageData);

/// @brief Convert image index data to nibble-sized values
std::vector<uint8_t> convertDataToNibbles(const std::vector<uint8_t> &indices);

/// @brief Increase all image indices by 1
std::vector<uint8_t> incImageIndicesBy1(const std::vector<uint8_t> &imageData);

/// @brief Swap index in image data with index #0
std::vector<uint8_t> swapIndexToIndex0(const std::vector<uint8_t> &imageData, uint8_t oldIndex);

/// @brief Swap indices in image data according to an index table
std::vector<uint8_t> swapIndices(const std::vector<uint8_t> &imageData, const std::vector<uint8_t> &newIndices);

/// @brief Padd / fill up color map to nrOfColors
std::vector<Magick::Color> padColorMap(const std::vector<Magick::Color> &colorMap, uint32_t nrOfColors);
