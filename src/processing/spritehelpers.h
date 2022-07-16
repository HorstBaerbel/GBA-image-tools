// some image utility functions used by the tools
#pragma once

#include <cstdint>
#include <vector>

/// @brief Cut data to tileWidth * height pixel wide tiles. Width and height and tileWidth MUST be a multiple of 8!
std::vector<uint8_t> convertToWidth(const std::vector<uint8_t> &data, uint32_t width, uint32_t height, uint32_t bitsPerPixel, uint32_t tileWidth);

/// @brief Cut data to 8x8 pixel wide tiles and store per tile instead of per scanline.
/// Width and height MUST be a multiple of 8!
std::vector<uint8_t> convertToTiles(const std::vector<uint8_t> &data, uint32_t width, uint32_t height, uint32_t bitsPerPixel, uint32_t tileWidth = 8, uint32_t tileHeight = 8);

/// @brief Cut data to 8x8 pixel wide tiles and store per sprite instead of per scanline.
/// Width and height MUST be a multiple of 8 and of spriteWidth and spriteHeight.
std::vector<uint8_t> convertToSprites(const std::vector<uint8_t> &src, uint32_t width, uint32_t height, uint32_t bitsPerPixel, uint32_t spriteWidth, uint32_t spriteHeight);

/// @brief Build a screen and tile map from tile data, storing only unique tiles. Only max. 1024 unique tiles allowed!
/// Source data MUST have been converted to tiles already and width and height MUST be a multiple of 8!
/// Will detect horizontally, vertically and horizontally+vertically flipped tiles and will set the map index flip flags accordingly (if detectFlips == true)
/// @return Returns (screen map, unique tile map)
std::pair<std::vector<uint16_t>, std::vector<uint8_t>> buildUniqueTileMap(const std::vector<uint8_t> &src, uint32_t width, uint32_t height, uint32_t bitsPerPixel, bool detectFlips, uint32_t tileWidth = 8, uint32_t tileHeight = 8);
