// some image utility functions used by the tools
#pragma once

#include <cstdint>
#include <vector>
#include <Magick++.h>

using namespace Magick;

/// @brief Get ImageMagick image data (palette or truecolor) as raw data bytes.
/// If the palette has < 16 entries, two (nibble-sized) indices will be combined into a byte.
std::vector<uint8_t> getImageData(const Image &img);

/// @brief Increase all image indices by 1
std::vector<uint8_t> incImageIndicesBy1(const std::vector<uint8_t> &imageData);

/// @brief Swap index in image data with 0
std::vector<uint8_t> swapIndexToIndex0(std::vector<uint8_t> &imageData, uint8_t oldIndex);

/// @brief Swap indices in image data according to index table
std::vector<uint8_t> swapIndices(std::vector<uint8_t> &imageData, const std::vector<uint8_t> &newIndices);
