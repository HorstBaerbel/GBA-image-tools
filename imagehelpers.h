// some image utility functions used by the tools
#pragma once

#include <cstdint>
#include <vector>
#include <Magick++.h>

using namespace Magick;

/// @brief Get ImageMagick image data (palette or truecolor) as raw data bytes.
std::vector<uint8_t> getImageData(const Image &img);

/// @brief Convert image index data to nibble-sized values
std::vector<uint8_t> convertDataToNibbles(const std::vector<uint8_t> &indices);

/// @brief Increase all image indices by 1
std::vector<uint8_t> incImageIndicesBy1(const std::vector<uint8_t> &imageData);

/// @brief Swap index in image data with index #0
std::vector<uint8_t> swapIndexToIndex0(const std::vector<uint8_t> &imageData, uint8_t oldIndex);

/// @brief Swap indices in image data according to an index table
std::vector<uint8_t> swapIndices(const std::vector<uint8_t> &imageData, const std::vector<uint8_t> &newIndices);

/// @brief Padd / fill up color map to nrOfColors
std::vector<Color> padColorMap(const std::vector<Color> &colorMap, uint32_t nrOfColors);
