#pragma once

#include "color/xrgb8888.h"

#include <cstdint>
#include <vector>

/// @brief Cut data to tileWidth * height pixel wide tiles. Width and height and tileWidth MUST be a multiple of 8!
/// Moves from left to right first, then top to bottom.
auto convertToWidth(const std::vector<Color::XRGB8888> &data, uint32_t width, uint32_t height, uint32_t tileWidth = 8) -> std::vector<Color::XRGB8888>;

/// @brief Cut data to 8x8 pixel wide tiles and store per tile instead of per scanline.
/// Width and height MUST be a multiple of 8!
/// Moves from left to right first, then top to bottom.
auto convertToTiles(const std::vector<Color::XRGB8888> &data, uint32_t width, uint32_t height, uint32_t tileWidth = 8, uint32_t tileHeight = 8) -> std::vector<Color::XRGB8888>;

/// @brief Cut data to 8x8 pixel wide tiles and store per sprite instead of per scanline.
/// Width and height MUST be a multiple of 8 and of spriteWidth and spriteHeight.
/// Moves from left to right first, then top to bottom
auto convertToSprites(const std::vector<Color::XRGB8888> &data, uint32_t width, uint32_t height, uint32_t spriteWidth, uint32_t spriteHeight) -> std::vector<Color::XRGB8888>;

/// @brief Build a screen and tile map from tile data, storing only unique tiles. Only max. 1024 unique tiles allowed!
/// Source data MUST have been converted to tiles already and width and height MUST be a multiple of 8!
/// Moves from left to right first, then top to bottom.
/// @param detectFlips Pass true to detect horizontally, vertically and horizontally+vertically flipped tiles and will set the map index flip flags accordingly.
/// @return Returns (screen map, unique tile map)
auto buildUniqueTileMap(const std::vector<Color::XRGB8888> &data, uint32_t width, uint32_t height, bool detectFlips, uint32_t tileWidth = 8, uint32_t tileHeight = 8) -> std::pair<std::vector<uint16_t>, std::vector<Color::XRGB8888>>;
