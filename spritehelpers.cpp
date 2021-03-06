#include "spritehelpers.h"

#include <cstring>

std::vector<uint8_t> convertToWidth(const std::vector<uint8_t> &src, uint32_t width, uint32_t height, uint32_t bitsPerPixel, uint32_t tileWidth)
{
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
