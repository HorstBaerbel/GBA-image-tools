#include "dxt.h"

#include "color/colorhelpers.h"
#include "color/conversions.h"
#include "color/rgb565.h"
#include "color/rgbf.h"
#include "color/xrgb1555.h"
#include "color/xrgb8888.h"
#include "dxttables.h"
#include "exception.h"
#include "math/linefit.h"

#include <Eigen/Core>
#include <Eigen/Dense>

#include <array>
#include <deque>
#include <iostream>

using namespace Color;

// This is basically the "range fit" method from here: http://www.sjbrown.co.uk/2006/01/19/dxt-compression-techniques/
std::vector<uint8_t> encodeBlockDXT(const Color::XRGB8888 *start, const uint32_t pixelsPerScanline, const bool asRGB565)
{
    REQUIRE(pixelsPerScanline % 4 == 0, std::runtime_error, "Image width must be a multiple of 4 for DXT compression");
    // get block colors for all 16 pixels
    std::array<RGBf, 16> colors;
    auto cIt = colors.begin();
    auto pixel = start;
    for (int y = 0; y < 4; y++)
    {
        *cIt++ = convertTo<RGBf>(pixel[0]);
        *cIt++ = convertTo<RGBf>(pixel[1]);
        *cIt++ = convertTo<RGBf>(pixel[2]);
        *cIt++ = convertTo<RGBf>(pixel[3]);
        pixel += pixelsPerScanline;
    }
    // calculate line fit through RGB color space
    auto originAndAxis = lineFit(colors);
    // calculate signed distance from origin
    std::vector<float> distanceFromOrigin(16);
    std::transform(colors.cbegin(), colors.cend(), distanceFromOrigin.begin(), [axis = originAndAxis.second](const auto &color)
                   { return color.dot(axis); });
    // get the distance of endpoints c0 and c1 on line
    auto minMaxDistance = std::minmax_element(distanceFromOrigin.cbegin(), distanceFromOrigin.cend());
    auto indexC0 = std::distance(distanceFromOrigin.cbegin(), minMaxDistance.first);
    auto indexC1 = std::distance(distanceFromOrigin.cbegin(), minMaxDistance.second);
    // get colors c0 and c1 on line and round to grid
    auto c0 = RGBf::roundTo(colors[indexC0], asRGB565 ? RGB565::Max : XRGB1555::Max);
    auto c1 = RGBf::roundTo(colors[indexC1], asRGB565 ? RGB565::Max : XRGB1555::Max);
    RGBf endpoints[4] = {c0, c1, {}, {}};
    /*if (toPixel(endpoints[0]) > toPixel(endpoints[1]))
    {
        std::swap(c0, c1);
        std::swap(endpoints[0], endpoints[1]);
    }*/
    // calculate intermediate colors c2 and c3 (rounded like in decoder)
    endpoints[2] = RGBf::roundTo(RGBf((c0.cwiseProduct(RGBf(2, 2, 2)) + c1).cwiseQuotient(RGBf(3, 3, 3))), asRGB565 ? RGB565::Max : XRGB1555::Max);
    endpoints[3] = RGBf::roundTo(RGBf((c0 + c1.cwiseProduct(RGBf(2, 2, 2))).cwiseQuotient(RGBf(3, 3, 3))), asRGB565 ? RGB565::Max : XRGB1555::Max);
    // calculate minimum distance for all colors to endpoints
    std::array<uint32_t, 16> bestIndices = {0};
    for (uint32_t ci = 0; ci < 16; ++ci)
    {
        if (ci == indexC0)
        {
            bestIndices[ci] = 0;
            continue;
        }
        else if (ci == indexC1)
        {
            bestIndices[ci] = 1;
            continue;
        }
        // calculate minimum distance for each index for this color
        float bestColorDistance = std::numeric_limits<float>::max();
        for (uint32_t ei = 0; ei < 4; ++ei)
        {
            auto indexDistance = RGBf::mse(colors[ci], endpoints[ei]);
            // check if result improved
            if (bestColorDistance > indexDistance)
            {
                bestColorDistance = indexDistance;
                bestIndices[ci] = ei;
            }
        }
    }
    // build result data
    std::vector<uint8_t> result(2 * 2 + 16 * 2 / 8);
    // add color endpoints c0 and c1
    auto data16 = reinterpret_cast<uint16_t *>(result.data());
    *data16++ = asRGB565 ? static_cast<uint16_t>(convertTo<RGB565>(endpoints[0])) : static_cast<uint16_t>(convertTo<XRGB1555>(endpoints[0]));
    *data16++ = asRGB565 ? static_cast<uint16_t>(convertTo<RGB565>(endpoints[1])) : static_cast<uint16_t>(convertTo<XRGB1555>(endpoints[1]));
    // add index data in reverse
    uint32_t indices = 0;
    for (auto iIt = bestIndices.crbegin(); iIt != bestIndices.crend(); ++iIt)
    {
        indices = (indices << 2) | *iIt;
    }
    *(reinterpret_cast<uint32_t *>(data16)) = indices;
    return result;
}

auto DXT::encodeDXT(const std::vector<Color::XRGB8888> &image, const uint32_t width, const uint32_t height, const bool asRGB565) -> std::vector<uint8_t>
{
    REQUIRE(width % 4 == 0, std::runtime_error, "Image width must be a multiple of 4 for DXT compression");
    REQUIRE(height % 4 == 0, std::runtime_error, "Image height must be a multiple of 4 for DXT compression");
    // compress to DXT1. we get 8 bytes per 4x4 block / 16 pixels
    const auto yStride = width * 8 / 16;
    const auto nrOfBlocks = width / 4 * height / 4;
    std::vector<uint8_t> dxtData(nrOfBlocks * 8);
#pragma omp parallel for
    for (int y = 0; y < height; y += 4)
    {
        for (uint32_t x = 0; x < width; x += 4)
        {
            auto block = encodeBlockDXT(image.data() + y * width + x, width, asRGB565);
            std::copy(block.cbegin(), block.cend(), std::next(dxtData.begin(), y * yStride + x * 8 / 4));
        }
    }
    // split data into colors and indices for better compression
    std::vector<uint8_t> data(nrOfBlocks * 8);
    auto colorPtr16 = reinterpret_cast<uint16_t *>(data.data());
    auto indexPtr16 = reinterpret_cast<uint16_t *>(data.data() + nrOfBlocks * 4);
    auto srcPtr16 = reinterpret_cast<const uint16_t *>(dxtData.data());
    for (uint32_t i = 0; i < nrOfBlocks; i++)
    {
        *colorPtr16++ = *srcPtr16++;
        *colorPtr16++ = *srcPtr16++;
        *indexPtr16++ = *srcPtr16++;
        *indexPtr16++ = *srcPtr16++;
    }

    return data;
}

auto DXT::decodeDXT(const std::vector<uint8_t> &data, const uint32_t width, const uint32_t height, const bool asRGB565) -> std::vector<Color::XRGB8888>
{
    REQUIRE(width % 4 == 0, std::runtime_error, "Image width must be a multiple of 4 for DXT decompression");
    REQUIRE(height % 4 == 0, std::runtime_error, "Image height must be a multiple of 4 for DXT decompression");
    const auto nrOfBlocks = data.size() / 8;
    REQUIRE(nrOfBlocks == width / 4 * height / 4, std::runtime_error, "Data size does not match image size");
    std::vector<Color::XRGB8888> result(width * height);
    // set up pointer to source block data
    auto color16 = reinterpret_cast<const uint16_t *>(data.data());
    auto indices32 = reinterpret_cast<const uint32_t *>(data.data() + nrOfBlocks * 4);
    for (std::size_t y = 0; y < height; y += 4)
    {
        for (std::size_t x = 0; x < width; x += 4)
        {
            // read colors c0 and c1
            std::array<Color::XRGB8888, 4> colors;
            const uint16_t c0 = *color16++;
            const uint16_t c1 = *color16++;
            colors[0] = asRGB565 ? convertTo<Color::XRGB8888>(Color::RGB565(c0)) : convertTo<Color::XRGB8888>(Color::XRGB1555(c0));
            colors[1] = asRGB565 ? convertTo<Color::XRGB8888>(Color::RGB565(c1)) : convertTo<Color::XRGB8888>(Color::XRGB1555(c1));
            // calculate intermediate colors c2 and c3 using tables
            uint32_t c2c3 = 0;
            if (asRGB565)
            {
                uint32_t b = ((c0 & 0xF800) >> 6) | ((c1 & 0xF800) >> 11);
                uint32_t g = ((c0 & 0x7E0) << 1) | ((c1 & 0x7E0) >> 5);
                uint32_t r = ((c0 & 0x1F) << 5) | (c1 & 0x1F);
                c2c3 = (DXTTables::C2C3_5bit[b] << 11) | (DXTTables::C2C3_6bit[g] << 5) | DXTTables::C2C3_5bit[r];
            }
            else
            {
                uint32_t b = ((c0 & 0x7C00) >> 5) | ((c1 & 0x7C00) >> 10);
                uint32_t g = (c0 & 0x3E0) | ((c1 & 0x3E0) >> 5);
                uint32_t r = ((c0 & 0x1F) << 5) | (c1 & 0x1F);
                c2c3 = (DXTTables::C2C3_5bit[b] << 10) | (DXTTables::C2C3_5bit[g] << 5) | DXTTables::C2C3_5bit[r];
            }
            const uint16_t c2 = (c2c3 & 0x0000FFFF);
            const uint16_t c3 = ((c2c3 & 0xFFFF0000) >> 16);
            colors[2] = asRGB565 ? convertTo<Color::XRGB8888>(Color::RGB565(c2)) : convertTo<Color::XRGB8888>(Color::XRGB1555(c2));
            colors[3] = asRGB565 ? convertTo<Color::XRGB8888>(Color::RGB565(c3)) : convertTo<Color::XRGB8888>(Color::XRGB1555(c3));
            // read pixel color indices
            uint32_t indices = *indices32++;
            // and decode colors
            auto dst = result.data() + y * width + x;
            for (std::size_t by = 0; by < 4; by++)
            {
                for (std::size_t bx = 0; bx < 4; bx++)
                {
                    *dst++ = colors[indices & 0x03];
                    indices >>= 2;
                }
                dst += width - 4;
            }
        }
    }
    return result;
}