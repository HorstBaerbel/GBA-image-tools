#include "spritehelpers.h"

#include "exception.h"

#include <cstring>
#include <map>
#include <array>

std::vector<uint8_t> convertToWidth(const std::vector<uint8_t> &src, uint32_t width, uint32_t height, uint32_t bitsPerPixel, uint32_t tileWidth)
{
    bitsPerPixel = bitsPerPixel == 15 ? 16 : bitsPerPixel;
    REQUIRE(bitsPerPixel == 4 || bitsPerPixel == 8 || bitsPerPixel == 16, std::runtime_error, "Bits per pixel must be in [4, 8, 15, 16]");
    REQUIRE(tileWidth % 8 == 0, std::runtime_error, "Tile width must be divisible by 8");
    REQUIRE(width % 8 == 0 && height % 8 == 0, std::runtime_error, "Width and height must be divisible by 8");
    std::vector<uint8_t> dst(src.size());
    const uint32_t bytesPerTileLine = bitsPerPixel * (tileWidth / 8);
    const uint32_t bytesPerSrcLine = (width * bitsPerPixel) / 8;
    uint8_t *dstData = dst.data();
    for (uint32_t blockX = 0; blockX < width; blockX += tileWidth)
    {
        const uint8_t *srcLine = src.data() + (blockX * bitsPerPixel) / 8;
        for (uint32_t tileY = 0; tileY < height; ++tileY)
        {
            std::memcpy(dstData, srcLine, bytesPerTileLine);
            dstData += bytesPerTileLine;
            srcLine += bytesPerSrcLine;
        }
    }
    return dst;
}

std::vector<uint8_t> convertToTiles(const std::vector<uint8_t> &src, uint32_t width, uint32_t height, uint32_t bitsPerPixel, uint32_t tileWidth, uint32_t tileHeight)
{
    bitsPerPixel = bitsPerPixel == 15 ? 16 : bitsPerPixel;
    REQUIRE(bitsPerPixel == 4 || bitsPerPixel == 8 || bitsPerPixel == 16, std::runtime_error, "Bits per pixel must be in [4, 8, 15, 16]");
    REQUIRE(tileWidth % 8 == 0 && tileHeight % 8 == 0, std::runtime_error, "Tile width and height must be divisible by 8");
    REQUIRE(width % 8 == 0 && height % 8 == 0, std::runtime_error, "Width and height must be divisible by 8");
    std::vector<uint8_t> dst(src.size());
    const uint32_t bytesPerTileLine = bitsPerPixel * (tileWidth / 8);
    const uint32_t bytesPerSrcLine = (width * bitsPerPixel) / 8;
    uint8_t *dstData = dst.data();
    for (uint32_t blockY = 0; blockY < height; blockY += tileHeight)
    {
        const uint8_t *srcBlock = src.data() + blockY * bytesPerSrcLine;
        for (uint32_t blockX = 0; blockX < width; blockX += tileWidth)
        {
            const uint8_t *srcLine = srcBlock + (blockX * bitsPerPixel) / 8;
            for (uint32_t tileY = 0; tileY < tileHeight; ++tileY)
            {
                std::memcpy(dstData, srcLine, bytesPerTileLine);
                dstData += bytesPerTileLine;
                srcLine += bytesPerSrcLine;
            }
        }
    }
    return dst;
}

std::vector<uint8_t> convertToSprites(const std::vector<uint8_t> &src, uint32_t width, uint32_t height, uint32_t bitsPerPixel, uint32_t spriteWidth, uint32_t spriteHeight)
{
    bitsPerPixel = bitsPerPixel == 15 ? 16 : bitsPerPixel;
    REQUIRE(bitsPerPixel == 4 || bitsPerPixel == 8 || bitsPerPixel == 16, std::runtime_error, "Bits per pixel must be in [4, 8, 15, 16]");
    REQUIRE(spriteWidth % 8 == 0 && spriteHeight % 8 == 0, std::runtime_error, "Sprite width and height must be divisible by 8");
    REQUIRE(width % 8 == 0 && height % 8 == 0, std::runtime_error, "Width and height must be divisible by 8");
    // convert to tiles first
    auto tileData = convertToTiles(src, width, height, bitsPerPixel);
    // now convert to sprites
    std::vector<uint8_t> dst(src.size());
    const uint32_t bytesPerTile = bitsPerPixel * 8;
    const uint32_t bytesPerSrcLine = (width * bitsPerPixel) / 8;
    const uint32_t spritesHorizontal = width / spriteWidth;
    const uint32_t spritesVertical = height / spriteHeight;
    const uint32_t spriteTileWidth = spriteWidth / 8;
    const uint32_t spriteTileHeight = spriteHeight / 8;
    const uint32_t bytesPerSpriteLine = spriteTileWidth * bytesPerTile;
    uint8_t *dstData = dst.data();
    for (uint32_t spriteY = 0; spriteY < spritesVertical; ++spriteY)
    {
        const uint8_t *srcBlock = src.data() + spriteY * spriteHeight * bytesPerSrcLine;
        for (uint32_t spriteX = 0; spriteX < spritesHorizontal; ++spriteX)
        {
            const uint8_t *srcTile = srcBlock + spriteX * bytesPerSpriteLine;
            for (uint32_t tileY = 0; tileY < spriteTileHeight; ++tileY)
            {
                std::memcpy(dstData, srcTile, bytesPerSpriteLine);
                dstData += bytesPerSpriteLine;
                srcTile += bytesPerSrcLine * 8;
            }
        }
    }
    return dst;
}

// Hash tile block 4 different ways: normal, flipped horizontally, flipped vertically, flipped in both directions
// Uses modified DJB2 hash functions. Not terribly efficient
std::array<uint32_t, 4> hashTileBlock(std::vector<uint8_t>::const_iterator src, uint32_t columns, uint32_t rows, uint32_t bytesPerRow)
{
    std::array<uint32_t, 4> hash = {15359, 11447, 6991, 5381};
    // hash normally
    auto src0 = src;
    for (int32_t i = 0; i < (columns * rows); i++)
    {
        hash[0] = hash[0] * 53 + *src0++;
    }
    // hash flipped horizontally
    for (int32_t y = 0; y < rows; y++)
    {
        auto src1 = std::next(src, y * bytesPerRow + bytesPerRow - 1);
        for (int32_t x = 0; x < columns; x++)
        {
            hash[1] = hash[1] * 43 + *src1--;
        }
    }
    // hash flipped vertically
    for (int32_t y = rows - 1; y >= 0; y--)
    {
        auto src2 = std::next(src, y * bytesPerRow);
        for (int32_t x = 0; x < columns; x++)
        {
            hash[2] = hash[2] * 37 + *src2++;
        }
    }
    // hash flipped both ways
    for (int32_t y = rows - 1; y >= 0; y--)
    {
        auto src3 = std::next(src, y * bytesPerRow + bytesPerRow - 1);
        for (int32_t x = 0; x < columns; x++)
        {
            hash[3] = hash[3] * 29 + *src3--;
        }
    }
    return hash;
}

std::pair<std::vector<uint16_t>, std::vector<uint8_t>> buildUniqueTileMap(const std::vector<uint8_t> &src, uint32_t width, uint32_t height, uint32_t bitsPerPixel, bool detectFlips, uint32_t tileWidth, uint32_t tileHeight)
{
    bitsPerPixel = bitsPerPixel == 15 ? 16 : bitsPerPixel;
    REQUIRE(bitsPerPixel == 4 || bitsPerPixel == 8 || bitsPerPixel == 16, std::runtime_error, "Bits per pixel must be in [4, 8, 15, 16]");
    REQUIRE(tileWidth % 8 == 0 && tileHeight % 8 == 0, std::runtime_error, "Tile width and height must be divisible by 8");
    REQUIRE(width % 8 == 0 && height % 8 == 0, std::runtime_error, "Width and height must be divisible by 8");
    const uint32_t bytesPerTileRow = bitsPerPixel;
    const uint32_t bytesPerTile = 8 * bitsPerPixel * (tileWidth / 8);
    std::vector<uint16_t> dstScreen(width / tileWidth * height / tileHeight); // screen map
    std::vector<uint8_t> dstTiles;                                            // unique tile map
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
        auto tileHash = hashTileBlock(srcIt, tileWidth, tileHeight, bytesPerTileRow);
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
            dstTileHashes[tileHash[0]] = nrOfUniqueTiles;
            dstTileHashesH[tileHash[1]] = nrOfUniqueTiles | (1 << 10);
            dstTileHashesV[tileHash[2]] = nrOfUniqueTiles | (1 << 11);
            dstTileHashesHV[tileHash[3]] = nrOfUniqueTiles | (3 << 10);
            dstScreen[tileIndex] = nrOfUniqueTiles;
            nrOfUniqueTiles++;
            // copy new tile data to tile map
            std::copy(srcIt, std::next(srcIt, bytesPerTile), std::back_inserter(dstTiles));
        }
        srcIt = std::next(srcIt, bytesPerTile);
    }
    return {dstScreen, dstTiles};
}