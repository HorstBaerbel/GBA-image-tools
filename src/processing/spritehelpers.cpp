#include "spritehelpers.h"

#include "exception.h"

#include <algorithm>
#include <cstring>
#include <map>
#include <array>

auto convertToWidth(const std::vector<Color::XRGB8888> &data, uint32_t width, uint32_t height, uint32_t tileWidth) -> std::vector<Color::XRGB8888>
{
    REQUIRE(data.size() == width * height, std::runtime_error, "Data size must be == width * height");
    REQUIRE(tileWidth % 8 == 0, std::runtime_error, "Tile width must be divisible by 8");
    REQUIRE(width % 8 == 0 && height % 8 == 0, std::runtime_error, "Width and height must be divisible by 8");
    std::vector<Color::XRGB8888> dst(data.size());
    auto dstData = dst.begin();
    for (uint32_t blockX = 0; blockX < width; blockX += tileWidth)
    {
        auto srcLine = std::next(data.cbegin(), blockX);
        for (uint32_t tileY = 0; tileY < height; ++tileY)
        {
            std::copy_n(srcLine, tileWidth, dstData);
            dstData = std::next(dstData, tileWidth);
            srcLine = std::next(srcLine, width);
        }
    }
    return dst;
}

auto convertToTiles(const std::vector<Color::XRGB8888> &data, uint32_t width, uint32_t height, uint32_t tileWidth, uint32_t tileHeight) -> std::vector<Color::XRGB8888>
{
    REQUIRE(data.size() == width * height, std::runtime_error, "Data size must be == width * height");
    REQUIRE(tileWidth % 8 == 0 && tileHeight % 8 == 0, std::runtime_error, "Tile width and height must be divisible by 8");
    REQUIRE(width % 8 == 0 && height % 8 == 0, std::runtime_error, "Width and height must be divisible by 8");
    std::vector<Color::XRGB8888> dst(data.size());
    auto dstData = dst.begin();
    for (uint32_t blockY = 0; blockY < height; blockY += tileHeight)
    {
        auto srcBlock = std::next(data.cbegin(), blockY * width);
        for (uint32_t blockX = 0; blockX < width; blockX += tileWidth)
        {
            auto srcLine = std::next(srcBlock, blockX);
            for (uint32_t tileY = 0; tileY < tileHeight; ++tileY)
            {
                std::copy_n(srcLine, tileWidth, dstData);
                dstData = std::next(dstData, tileWidth);
                srcLine = std::next(srcLine, width);
            }
        }
    }
    return dst;
}

auto convertToSprites(const std::vector<Color::XRGB8888> &src, uint32_t width, uint32_t height, uint32_t spriteWidth, uint32_t spriteHeight) -> std::vector<Color::XRGB8888>
{
    REQUIRE(spriteWidth % 8 == 0 && spriteHeight % 8 == 0, std::runtime_error, "Sprite width and height must be divisible by 8");
    REQUIRE(width % 8 == 0 && height % 8 == 0, std::runtime_error, "Width and height must be divisible by 8");
    return convertToTiles(convertToWidth(src, width, height, spriteWidth), width, height);
}

// Hash tile block 4 different ways: normal, flipped horizontally, flipped vertically, flipped in both directions
// Uses modified DJB2 hash functions. Not terribly efficient
auto hashTileBlock(std::vector<Color::XRGB8888>::const_iterator src, uint32_t columns, uint32_t rows, uint32_t pixelsPerRow) -> std::array<uint32_t, 4>
{
    std::array<uint32_t, 4> hash = {15359, 11447, 6991, 5381};
    // hash normally
    auto src0 = src;
    for (int32_t i = 0; i < (columns * rows); i++, ++src0)
    {
        hash[0] = hash[0] * (11 + static_cast<uint32_t>(src0->R()));
        hash[0] = hash[0] * (31 + static_cast<uint32_t>(src0->G()));
        hash[0] = hash[0] * (53 + static_cast<uint32_t>(src0->B()));
    }
    // hash flipped horizontally
    for (int32_t y = 0; y < rows; y++)
    {
        auto src1 = std::next(src, y * pixelsPerRow + pixelsPerRow - 1);
        for (int32_t x = 0; x < columns; x++, --src1)
        {
            hash[1] = hash[1] * (7 + static_cast<uint32_t>(src1->R()));
            hash[1] = hash[1] * (61 + static_cast<uint32_t>(src1->G()));
            hash[1] = hash[1] * (43 + static_cast<uint32_t>(src1->B()));
        }
    }
    // hash flipped vertically
    for (int32_t y = rows - 1; y >= 0; y--)
    {
        auto src2 = std::next(src, y * pixelsPerRow);
        for (int32_t x = 0; x < columns; x++, ++src2)
        {
            hash[2] = hash[2] * (47 + static_cast<uint32_t>(src2->R()));
            hash[2] = hash[2] * (19 + static_cast<uint32_t>(src2->G()));
            hash[2] = hash[2] * (37 + static_cast<uint32_t>(src2->B()));
        }
    }
    // hash flipped both ways
    for (int32_t y = rows - 1; y >= 0; y--)
    {
        auto src3 = std::next(src, y * pixelsPerRow + pixelsPerRow - 1);
        for (int32_t x = 0; x < columns; x++, --src3)
        {
            hash[3] = hash[3] * (29 + static_cast<uint32_t>(src3->R()));
            hash[3] = hash[3] * (17 + static_cast<uint32_t>(src3->G()));
            hash[3] = hash[3] * (13 + static_cast<uint32_t>(src3->B()));
        }
    }
    return hash;
}

auto buildUniqueTileMap(const std::vector<Color::XRGB8888> &src, uint32_t width, uint32_t height, bool detectFlips, uint32_t tileWidth, uint32_t tileHeight) -> std::pair<std::vector<uint16_t>, std::vector<Color::XRGB8888>>
{
    REQUIRE(tileWidth % 8 == 0 && tileHeight % 8 == 0, std::runtime_error, "Tile width and height must be divisible by 8");
    REQUIRE(width % 8 == 0 && height % 8 == 0, std::runtime_error, "Width and height must be divisible by 8");
    const uint32_t pixelsPerTile = tileWidth * tileHeight;
    std::vector<uint16_t> dstScreen(width / tileWidth * height / tileHeight); // screen map
    std::vector<Color::XRGB8888> dstTiles;                                    // unique tile map
    uint16_t nrOfUniqueTiles = 0;                                             // # of tiles currently in tile map
    std::map<uint32_t, uint16_t> dstTileHashes;                               // map from hash values of tile pixels -> tile map index
    std::map<uint32_t, uint16_t> dstTileHashesH;                              // map from hash values of tile pixels swapped in H -> tile map index
    std::map<uint32_t, uint16_t> dstTileHashesV;                              // map from hash values of tile pixels swapped in V -> tile map index
    std::map<uint32_t, uint16_t> dstTileHashesHV;                             // map from hash values of tile pixels swapped in H&V -> tile map index
    // find screen map indices for all tiles while sorting out duplicates
    auto srcIt = src.cbegin();
    for (uint32_t tileIndex = 0; tileIndex < dstScreen.size(); tileIndex++)
    {
        // hash tile pixel block
        auto tileHash = hashTileBlock(srcIt, tileWidth, tileHeight, width);
        // check if tile pixel hash can be found in one of the hash maps
        if (auto tileIt = dstTileHashes.find(tileHash[0]); tileIt != dstTileHashes.cend())
        {
            dstScreen[tileIndex] = tileIt->second;
        }
        else if (auto tileIt = dstTileHashesH.find(tileHash[1]); detectFlips && tileIt != dstTileHashesH.cend())
        {
            dstScreen[tileIndex] = tileIt->second;
        }
        else if (auto tileIt = dstTileHashesV.find(tileHash[2]); detectFlips && tileIt != dstTileHashesV.cend())
        {
            dstScreen[tileIndex] = tileIt->second;
        }
        else if (auto tileIt = dstTileHashesHV.find(tileHash[3]); detectFlips && tileIt != dstTileHashesHV.cend())
        {
            dstScreen[tileIndex] = tileIt->second;
        }
        else
        {
            REQUIRE(nrOfUniqueTiles < 1024, std::runtime_error, "Too many unique tiles. Max 1023 tiles allowed");
            // tile not in map. add new tile index
            dstScreen[tileIndex] = nrOfUniqueTiles;
            // add this tiles index to all hash maps
            dstTileHashes[tileHash[0]] = nrOfUniqueTiles;
            dstTileHashesH[tileHash[1]] = nrOfUniqueTiles | (1 << 10);
            dstTileHashesV[tileHash[2]] = nrOfUniqueTiles | (1 << 11);
            dstTileHashesHV[tileHash[3]] = nrOfUniqueTiles | (3 << 10);
            nrOfUniqueTiles++;
            // copy new tile data to tile map
            std::copy(srcIt, std::next(srcIt, pixelsPerTile), std::back_inserter(dstTiles));
        }
        srcIt = std::next(srcIt, pixelsPerTile);
    }
    return std::make_pair(dstScreen, dstTiles);
}