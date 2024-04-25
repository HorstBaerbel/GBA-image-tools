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

    template <typename pixel_type>
    auto hashPixel(const typename std::vector<pixel_type>::const_iterator src, uint32_t hash, uint32_t prime) -> uint32_t
    {
        if constexpr (std::is_same<pixel_type, uint8_t>())
        {
            hash += prime * (263 + static_cast<uint32_t>(*src));
        }
        else
        {
            hash += prime * (2 + static_cast<uint32_t>(src->R()));
            hash += prime * (263 + static_cast<uint32_t>(src->G()));
            hash += prime * (521 + static_cast<uint32_t>(src->B()));
        }
        return hash;
    }

    // Hash tile block 4 different ways: normal, flipped horizontally, flipped vertically, flipped in both directions
    template <typename pixel_type>
    auto hashTileBlock(const typename std::vector<pixel_type>::const_iterator src, uint32_t columns, uint32_t rows, bool hashFlips) -> std::array<uint32_t, 4>
    {
        static const std::array<uint32_t, 256> primes = {1277, 1889, 1021, 1231, 1069, 239, 1789, 1913, 359, 1163, 151, 1861, 103, 47, 1567, 1693, 509, 1297, 467, 1571, 1613, 1097, 491, 1987, 349, 1787, 907, 967, 1213, 41, 1427, 1171, 109, 619, 499, 199, 1489, 857, 421, 1091, 1217, 1999, 13, 97, 257, 1187, 229, 1291, 761, 547, 1831, 1759, 751, 1327, 53, 43, 11, 1459, 1061, 71, 1637, 887, 293, 1601, 659, 1811, 431, 1973, 1871, 853, 409, 1559, 1721, 571, 1993, 787, 181, 1709, 1549, 557, 773, 653, 941, 983, 1867, 1151, 1483, 233, 107, 1621, 317, 733, 1399, 1511, 1879, 1303, 719, 1933, 997, 1259, 1951, 1033, 661, 977, 1409, 59, 127, 641, 701, 1523, 1901, 449, 1453, 1249, 37, 1753, 1367, 1847, 929, 457, 1949, 1579, 1201, 149, 1997, 877, 1451, 1583, 953, 859, 347, 227, 839, 367, 31, 1907, 1117, 271, 139, 911, 1193, 1627, 1123, 1051, 419, 797, 2, 1873, 283, 397, 1031, 131, 179, 23, 157, 263, 1747, 631, 1381, 379, 1039, 863, 1663, 1103, 1697, 313, 919, 1223, 1741, 1657, 1493, 607, 1087, 709, 1423, 1597, 727, 1321, 523, 569, 7, 1009, 167, 677, 563, 1429, 1619, 991, 1153, 73, 613, 383, 79, 17, 89, 211, 947, 643, 61, 811, 241, 829, 1607, 1931, 463, 1877, 373, 197, 599, 1109, 1783, 1471, 937, 1237, 401, 277, 353, 1319, 281, 389, 743, 1307, 587, 1093, 541, 1049, 83, 67, 163, 769, 1447, 1229, 1129, 193, 1487, 337, 617, 461, 647, 443, 883, 1063, 3, 1499, 1667, 1531, 1669, 251, 101, 823, 173, 439, 757, 1433, 1013, 311};
        static constexpr uint32_t piStart = 5;
        std::array<uint32_t, 4> hash = {5381, 5381, 5381, 5381};
        // hash normally
        uint32_t pi = piStart;
        auto src0 = src;
        for (int32_t y = 0; y < rows; y++)
        {
            for (int32_t x = 0; x < columns; x++, ++src0)
            {
                auto p = primes[pi++ % (primes.size() - 1)];
                hash[0] = hashPixel(src0, hash[0], p);
            }
        }
        if (hashFlips)
        {
            // hash flipped horizontally
            pi = piStart;
            for (int32_t y = 0; y < rows; y++)
            {
                auto src1 = std::next(src, y * columns + columns - 1);
                for (int32_t x = columns - 1; x >= 0; x--, --src1)
                {
                    auto p = primes[pi++ % (primes.size() - 1)];
                    hash[1] = hashPixel(src1, hash[1], p);
                }
            }
            // hash flipped vertically
            pi = piStart;
            for (int32_t y = rows - 1; y >= 0; y--)
            {
                auto src2 = std::next(src, y * columns);
                for (int32_t x = 0; x < columns; x++, ++src2)
                {
                    auto p = primes[pi++ % (primes.size() - 1)];
                    hash[2] = hashPixel(src2, hash[2], p);
                }
            }
            // hash flipped both ways
            pi = piStart;
            auto src3 = std::next(src, (rows - 1) * columns + columns - 1);
            for (int32_t y = 0; y < rows; y++)
            {
                for (int32_t x = columns - 1; x >= 0; x--, --src3)
                {
                    auto p = primes[pi++ % (primes.size() - 1)];
                    hash[3] = hashPixel(src3, hash[3], p);
                }
            }
        }
        return hash;
    }

    template <typename pixel_type>
    auto buildUniqueTileMap(const std::vector<pixel_type> &data, uint32_t width, uint32_t height, bool detectFlips, uint32_t tileWidth, uint32_t tileHeight) -> std::pair<std::vector<uint16_t>, std::vector<pixel_type>>
    {
        REQUIRE(data.size() == width * height, std::runtime_error, "Data size must be == width * height");
        REQUIRE(tileWidth % 8 == 0 && tileHeight % 8 == 0, std::runtime_error, "Tile width and height must be divisible by 8");
        REQUIRE(width % 8 == 0 && height % 8 == 0, std::runtime_error, "Width and height must be divisible by 8");
        const uint32_t pixelsPerTile = tileWidth * tileHeight;
        std::vector<uint16_t> dstScreen(width / tileWidth * height / tileHeight); // screen map
        std::vector<pixel_type> dstTiles;                                         // unique tile map
        uint16_t nrOfUniqueTiles = 0;                                             // # of tiles currently in tile map
        std::map<uint32_t, uint16_t> dstTileHashes;                               // map from hash values of tile pixels -> tile map index
        // find screen map indices for all tiles while sorting out duplicates
        auto srcIt = data.cbegin();
        for (uint32_t tileIndex = 0; tileIndex < dstScreen.size(); tileIndex++)
        {
            // hash tile pixel block
            auto tileHash = hashTileBlock(srcIt, tileWidth, tileHeight, detectFlips);
            // check if tile pixel hash can be found in one of the hash maps
            if (auto tileIt = dstTileHashes.find(tileHash[0]); tileIt != dstTileHashes.cend())
            {
                dstScreen[tileIndex] = tileIt->second;
            }
            else if (auto tileIt = dstTileHashes.find(tileHash[1]); detectFlips && tileIt != dstTileHashes.cend())
            {
                dstScreen[tileIndex] = tileIt->second;
            }
            else if (auto tileIt = dstTileHashes.find(tileHash[2]); detectFlips && tileIt != dstTileHashes.cend())
            {
                dstScreen[tileIndex] = tileIt->second;
            }
            else if (auto tileIt = dstTileHashes.find(tileHash[3]); detectFlips && tileIt != dstTileHashes.cend())
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
                if (detectFlips)
                {
                    dstTileHashes[tileHash[1]] = nrOfUniqueTiles | (1 << 10);
                    dstTileHashes[tileHash[2]] = nrOfUniqueTiles | (1 << 11);
                    dstTileHashes[tileHash[3]] = nrOfUniqueTiles | (3 << 10);
                }
                nrOfUniqueTiles++;
                // copy new tile data to tile map
                std::copy(srcIt, std::next(srcIt, pixelsPerTile), std::back_inserter(dstTiles));
            }
            srcIt = std::next(srcIt, pixelsPerTile);
        }
        return std::make_pair(dstScreen, dstTiles);
    }

    auto buildUniqueTileMap(const PixelData &data, uint32_t width, uint32_t height, bool detectFlips, uint32_t tileWidth, uint32_t tileHeight) -> std::pair<std::vector<uint16_t>, PixelData>
    {
        auto format = data.format();
        return std::visit([format, width, height, detectFlips, tileWidth, tileHeight](auto pixels) -> std::pair<std::vector<uint16_t>, PixelData>
                          { 
                            using T = std::decay_t<decltype(pixels)>;
                            if constexpr(std::is_same<T, std::vector<uint8_t>>() || std::is_same<T, std::vector<Color::XRGB1555>>() || std::is_same<T, std::vector<Color::RGB565>>() || std::is_same<T, std::vector<Color::XRGB8888>>())
                            {
                                auto tilemapAndPixels = buildUniqueTileMap(pixels, width, height, detectFlips, tileWidth, tileHeight);
                                return std::make_pair(tilemapAndPixels.first, PixelData(tilemapAndPixels.second, format));
                            }
                            THROW(std::runtime_error, "Color format must be Paletted8, XRGB1555, RGB565 or XRGB8888"); },
                          data.storage());
    }
}
