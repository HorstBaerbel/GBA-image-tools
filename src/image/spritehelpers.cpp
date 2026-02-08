#include "spritehelpers.h"

#include "exception.h"

#include <algorithm>
#include <array>
#include <cstring>
#include <map>

namespace Image
{

    template <typename pixel_type>
    auto convertToWidth(const std::vector<pixel_type> &data, uint32_t width, uint32_t height, uint32_t tileWidth) -> std::vector<pixel_type>
    {
        REQUIRE(data.size() == width * height, std::runtime_error, "Data size must be == width * height");
        REQUIRE(tileWidth % 8 == 0, std::runtime_error, "Tile width must be divisible by 8");
        REQUIRE(width % 8 == 0 && height % 8 == 0, std::runtime_error, "Width and height must be divisible by 8");
        std::vector<pixel_type> dst(data.size());
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

    auto convertToWidth(const PixelData &data, uint32_t width, uint32_t height, uint32_t tileWidth) -> PixelData
    {
        auto format = data.format();
        return std::visit([format, width, height, tileWidth](auto pixels) -> PixelData
                          { 
                            using T = std::decay_t<decltype(pixels)>;
                            if constexpr(std::is_same<T, std::vector<uint8_t>>() || std::is_same<T, std::vector<Color::XRGB1555>>() || std::is_same<T, std::vector<Color::RGB565>>() || std::is_same<T, std::vector<Color::XRGB8888>>())
                            {
                                return PixelData(convertToWidth(pixels, width, height, tileWidth), format);
                            }
                            THROW(std::runtime_error, "Color format must be Paletted8, XRGB1555, RGB565 or XRGB8888"); },
                          data.storage());
    }

    template <typename pixel_type>
    auto convertToTiles(const std::vector<pixel_type> &data, uint32_t width, uint32_t height, uint32_t tileWidth, uint32_t tileHeight) -> std::vector<pixel_type>
    {
        REQUIRE(data.size() == width * height, std::runtime_error, "Data size must be == width * height");
        REQUIRE(tileWidth % 8 == 0 && tileHeight % 8 == 0, std::runtime_error, "Tile width and height must be divisible by 8");
        REQUIRE(width % 8 == 0 && height % 8 == 0, std::runtime_error, "Width and height must be divisible by 8");
        std::vector<pixel_type> dst(data.size());
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

    auto convertToTiles(const PixelData &data, uint32_t width, uint32_t height, uint32_t tileWidth, uint32_t tileHeight) -> PixelData
    {
        auto format = data.format();
        return std::visit([format, width, height, tileWidth, tileHeight](auto pixels) -> PixelData
                          { 
                            using T = std::decay_t<decltype(pixels)>;
                            if constexpr(std::is_same<T, std::vector<uint8_t>>() || std::is_same<T, std::vector<Color::XRGB1555>>() || std::is_same<T, std::vector<Color::RGB565>>() || std::is_same<T, std::vector<Color::XRGB8888>>())
                            {
                                return PixelData(convertToTiles(pixels, width, height, tileWidth, tileHeight), format);
                            }
                            THROW(std::runtime_error, "Color format must be Paletted8, XRGB1555, RGB565 or XRGB8888"); },
                          data.storage());
    }

    auto convertToSprites(const PixelData &data, uint32_t width, uint32_t height, uint32_t spriteWidth, uint32_t spriteHeight) -> PixelData
    {
        REQUIRE(spriteWidth % 8 == 0 && spriteHeight % 8 == 0, std::runtime_error, "Sprite width and height must be divisible by 8");
        REQUIRE(width % 8 == 0 && height % 8 == 0, std::runtime_error, "Width and height must be divisible by 8");
        return convertToTiles(convertToWidth(data, width, height, spriteWidth), width, height);
    }

    // FNV-1a hash of a tile pixel, see: https://en.wikipedia.org/wiki/Fowler%E2%80%93Noll%E2%80%93Vo_hash_function#FNV-1a_hash
    template <typename pixel_type>
    auto hashPixel(const typename std::vector<pixel_type>::const_iterator src, uint64_t hash) -> uint64_t
    {
        if constexpr (std::is_same<pixel_type, uint8_t>())
        {
            hash ^= static_cast<uint64_t>(*src);
            hash *= 0x100000001b3;
        }
        else
        {
            hash ^= static_cast<uint64_t>(src->R());
            hash *= 0x100000001b3;
            hash ^= static_cast<uint64_t>(src->G());
            hash *= 0x100000001b3;
            hash ^= static_cast<uint64_t>(src->B());
            hash *= 0x100000001b3;
        }
        return hash;
    }

    // Hash tile block 4 different ways: normal, flipped horizontally, flipped vertically, flipped in both directions
    template <typename pixel_type>
    auto hashTileBlock(const typename std::vector<pixel_type>::const_iterator src, uint32_t columns, uint32_t rows, bool hashFlips) -> std::array<uint64_t, 4>
    {
        std::array<uint64_t, 4> hash = {0xcbf29ce484222325, 0xcbf29ce484222325, 0xcbf29ce484222325, 0xcbf29ce484222325};
        // hash normally
        auto src0 = src;
        for (int32_t y = 0; y < rows; y++)
        {
            for (int32_t x = 0; x < columns; x++, ++src0)
            {
                hash[0] = hashPixel<pixel_type>(src0, hash[0]);
            }
        }
        if (hashFlips)
        {
            // hash flipped horizontally
            for (int32_t y = 0; y < rows; y++)
            {
                auto src1 = std::next(src, y * columns + columns - 1);
                for (int32_t x = columns - 1; x >= 0; x--, --src1)
                {
                    hash[1] = hashPixel<pixel_type>(src1, hash[1]);
                }
            }
            // hash flipped vertically
            for (int32_t y = rows - 1; y >= 0; y--)
            {
                auto src2 = std::next(src, y * columns);
                for (int32_t x = 0; x < columns; x++, ++src2)
                {
                    hash[2] = hashPixel<pixel_type>(src2, hash[2]);
                }
            }
            // hash flipped both ways
            auto src3 = std::next(src, (rows - 1) * columns + columns - 1);
            for (int32_t y = 0; y < rows; y++)
            {
                for (int32_t x = columns - 1; x >= 0; x--, --src3)
                {
                    hash[3] = hashPixel<pixel_type>(src3, hash[3]);
                }
            }
        }
        return hash;
    }

    template <typename pixel_type>
    auto buildUniqueTileMap(const std::vector<std::vector<pixel_type>> &frames, uint32_t width, uint32_t height, bool detectFlips, uint32_t tileWidth, uint32_t tileHeight, uint32_t maxNrOfTiles = 1024) -> std::pair<std::vector<std::vector<uint16_t>>, std::vector<pixel_type>>
    {
        REQUIRE(frames.front().size() == width * height, std::runtime_error, "Data size must be == width * height");
        REQUIRE(tileWidth % 8 == 0 && tileHeight % 8 == 0, std::runtime_error, "Tile width and height must be divisible by 8");
        REQUIRE(width % 8 == 0 && height % 8 == 0, std::runtime_error, "Width and height must be divisible by 8");
        REQUIRE(maxNrOfTiles > 0 && maxNrOfTiles <= (1 << 14), std::runtime_error, "Max. number of tiles must be > 0 and <= " << (1 << 14));
        const uint32_t TileIndexBits = (std::log2(maxNrOfTiles - 1)) + 1;
        const uint32_t pixelsPerTile = tileWidth * tileHeight;
        std::vector<std::vector<uint16_t>> dstScreens; // screen maps for individual frames
        std::vector<pixel_type> dstTiles;              // unique tile map
        uint32_t nrOfUniqueTiles = 0;                  // # of tiles currently in tile map
        std::map<uint64_t, uint16_t> dstTileHashes;    // map from hash values of tile pixels -> tile map index
        for (const auto &framePixels : frames)
        {
            std::vector<uint16_t> frameScreen(width / tileWidth * height / tileHeight); // screen map for frame
            auto pixelIt = framePixels.cbegin();
            // find screen map indices for all tiles while sorting out duplicates
            for (uint32_t tileIndex = 0; tileIndex < frameScreen.size(); tileIndex++)
            {
                // hash tile pixel block
                auto tileHash = hashTileBlock<pixel_type>(pixelIt, tileWidth, tileHeight, detectFlips);
                // check if tile pixel hash can be found in one of the hash maps
                if (auto tileIt = dstTileHashes.find(tileHash[0]); tileIt != dstTileHashes.cend())
                {
                    frameScreen[tileIndex] = tileIt->second;
                }
                else if (auto tileIt = dstTileHashes.find(tileHash[1]); detectFlips && tileIt != dstTileHashes.cend())
                {
                    frameScreen[tileIndex] = tileIt->second;
                }
                else if (auto tileIt = dstTileHashes.find(tileHash[2]); detectFlips && tileIt != dstTileHashes.cend())
                {
                    frameScreen[tileIndex] = tileIt->second;
                }
                else if (auto tileIt = dstTileHashes.find(tileHash[3]); detectFlips && tileIt != dstTileHashes.cend())
                {
                    frameScreen[tileIndex] = tileIt->second;
                }
                else
                {
                    REQUIRE(nrOfUniqueTiles < maxNrOfTiles, std::runtime_error, "Too many unique tiles. Max " << (maxNrOfTiles - 1) << " tiles allowed");
                    // tile not in map. add new tile index
                    frameScreen[tileIndex] = nrOfUniqueTiles;
                    // add this tiles index to all hash maps
                    dstTileHashes[tileHash[0]] = nrOfUniqueTiles;
                    if (detectFlips)
                    {
                        dstTileHashes[tileHash[1]] = nrOfUniqueTiles | (1 << (TileIndexBits));
                        dstTileHashes[tileHash[2]] = nrOfUniqueTiles | (1 << (TileIndexBits + 1));
                        dstTileHashes[tileHash[3]] = nrOfUniqueTiles | (3 << (TileIndexBits));
                    }
                    nrOfUniqueTiles++;
                    // copy new tile data to tile map
                    std::copy(pixelIt, std::next(pixelIt, pixelsPerTile), std::back_inserter(dstTiles));
                }
                pixelIt = std::next(pixelIt, pixelsPerTile);
            }
            dstScreens.push_back(frameScreen);
        }
        return std::make_pair(dstScreens, dstTiles);
    }

    auto buildUniqueTileMap(const PixelData &data, uint32_t width, uint32_t height, bool detectFlips, uint32_t tileWidth, uint32_t tileHeight) -> std::pair<std::vector<uint16_t>, PixelData>
    {
        const auto format = data.format();
        return std::visit([format, width, height, detectFlips, tileWidth, tileHeight](auto frame) -> std::pair<std::vector<uint16_t>, PixelData>
                          { 
                            using T = std::decay_t<decltype(frame)>;
                            if constexpr(std::is_same<T, std::vector<uint8_t>>() || std::is_same<T, std::vector<Color::XRGB1555>>() || std::is_same<T, std::vector<Color::RGB565>>() || std::is_same<T, std::vector<Color::XRGB8888>>())
                            {
                                auto tilemapAndPixels = buildUniqueTileMap(std::vector<T>{frame}, width, height, detectFlips, tileWidth, tileHeight, 1024);
                                return std::make_pair(tilemapAndPixels.first.front(), PixelData(tilemapAndPixels.second, format));
                            }
                            THROW(std::runtime_error, "Color format must be Paletted8, XRGB1555, RGB565 or XRGB8888"); },
                          data.storage());
    }

    auto buildCommonTileMap(const std::vector<PixelData> &data, uint32_t width, uint32_t height, bool detectFlips, uint32_t tileWidth, uint32_t tileHeight) -> std::pair<std::vector<std::vector<uint16_t>>, PixelData>
    {
        const auto format = data.front().format();
        return std::visit([&data, format, width, height, detectFlips, tileWidth, tileHeight](auto first) -> std::pair<std::vector<std::vector<uint16_t>>, PixelData>
                          { 
                            using T = std::decay_t<decltype(first)>;
                            if constexpr(std::is_same<T, std::vector<uint8_t>>() || std::is_same<T, std::vector<Color::XRGB1555>>() || std::is_same<T, std::vector<Color::RGB565>>() || std::is_same<T, std::vector<Color::XRGB8888>>())
                            {
                                std::vector<T> frames;
                                for (auto dIt = data.cbegin(); dIt != data.cend(); ++dIt)
                                {
                                    frames.push_back(dIt->convertData<typename T::value_type>());
                                }
                                auto tilemapAndPixels = buildUniqueTileMap(frames, width, height, detectFlips, tileWidth, tileHeight, 16384);
                                return std::make_pair(tilemapAndPixels.first, PixelData(tilemapAndPixels.second, format));
                                return {};
                            }
                            THROW(std::runtime_error, "Color format must be Paletted8, XRGB1555, RGB565 or XRGB8888"); },
                          data.front().storage());
    }
}
