// some image utility functions used by the tools
#pragma once

#include "color/colorformat.h"
#include "color/rgbf.h"

#include <cstdint>
#include <vector>
#include <Magick++.h>

/// @brief Get ImageMagick image data (palette or truecolor) as raw data bytes
/// The format returned are Color::Format::Palettes8 for paletted images and Color::Format::RGB888 for truecolor images
std::pair<std::vector<uint8_t>, Color::Format> getImageData(const Magick::Image &img);

/// @brief Get ImageMagick truecolor image data as raw data words
/// The format returned is Color::Format::XRGB888. It will fail for paletted images
std::pair<std::vector<uint32_t>, Color::Format> getImageDataXRGB888(const Magick::Image &img);

/// @brief Get ImageMagick truecolor image data as RGBf structs
/// The format returned is Color::Format::RGBf. It will fail for paletted images
std::pair<std::vector<Color::RGBf>, Color::Format> getImageDataRGBf(const Magick::Image &img);

/// @brief Get color map from ImageMagick Image
std::vector<Magick::Color> getColorMap(const Magick::Image &img);

/// @brief set color map in an ImageMagick Image
void setColorMap(Magick::Image &img, const std::vector<Magick::Color> &colorMap);

/// @brief Convert image index data to 1-bit-sized values. Data must be divisible by 8
std::vector<uint8_t> convertDataTo1Bit(const std::vector<uint8_t> &indices);

/// @brief Convert image index data to 2-bit-sized values. Data must be divisible by 4
std::vector<uint8_t> convertDataTo2Bit(const std::vector<uint8_t> &indices);

/// @brief Convert image index data to nibble-sized values. Data must be divisible by 2
std::vector<uint8_t> convertDataTo4Bit(const std::vector<uint8_t> &indices);

/// @brief Increase all image indices by 1
std::vector<uint8_t> incImageIndicesBy1(const std::vector<uint8_t> &imageData);

/// @brief Swap index in image data with index #0
std::vector<uint8_t> swapIndexToIndex0(const std::vector<uint8_t> &imageData, uint8_t oldIndex);

/// @brief Swap indices in image data according to an index table
std::vector<uint8_t> swapIndices(const std::vector<uint8_t> &imageData, const std::vector<uint8_t> &newIndices);

/// @brief Padd / fill up color map to nrOfColors
std::vector<Magick::Color> padColorMap(const std::vector<Magick::Color> &colorMap, uint32_t nrOfColors);
