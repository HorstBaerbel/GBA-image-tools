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
#include <numeric>

using namespace Color;

#define CLUSTER_FIT

// Fit a line through colors passed using SVD
auto dxtLineFit(const std::vector<RGBf> &colors, const bool asRGB565) -> std::vector<RGBf>
{
    // calculate initial line fit through RGB color space
    auto originAndAxis = lineFit(colors);
    // calculate signed distance along line from origin
    std::vector<float> distanceOnLine(16);
    std::transform(colors.cbegin(), colors.cend(), distanceOnLine.begin(), [axis = originAndAxis.second](const auto &color)
                   { return color.dot(axis); });
    // get the distance of endpoints c0 and c1 on line
    auto minMaxDistance = std::minmax_element(distanceOnLine.cbegin(), distanceOnLine.cend());
    auto i0 = std::distance(distanceOnLine.cbegin(), minMaxDistance.first);
    auto i1 = std::distance(distanceOnLine.cbegin(), minMaxDistance.second);
    // get colors c0 and c1 on line
    std::vector<RGBf> c(4);
    c[0] = RGBf::roundTo(colors[i0], asRGB565 ? RGB565::Max : XRGB1555::Max);
    c[1] = RGBf::roundTo(colors[i1], asRGB565 ? RGB565::Max : XRGB1555::Max);
    // calculate intermediate colors c2 and c3
    c[2] = RGBf::roundTo(RGBf((c[0].cwiseProduct(RGBf(2, 2, 2)) + c[1]).cwiseQuotient(RGBf(3, 3, 3))), asRGB565 ? RGB565::Max : XRGB1555::Max);
    c[3] = RGBf::roundTo(RGBf((c[0] + c[1].cwiseProduct(RGBf(2, 2, 2))).cwiseQuotient(RGBf(3, 3, 3))), asRGB565 ? RGB565::Max : XRGB1555::Max);
    return c;
}

auto calculateError(const std::vector<RGBf> &endpoints, const std::vector<RGBf> &colors, const bool asRGB565) -> float
{
    // calculate minimum distance for all colors to endpoints and calculate error to that endpoint
    float error = 0.0F;
    for (uint32_t ci = 0; ci < 16; ++ci)
    {
        // calculate minimum distance for each index for this color
        float bestColorDistance = std::numeric_limits<float>::max();
        for (uint32_t ei = 0; ei < 4; ++ei)
        {
            auto indexDistance = RGBf::mse(colors[ci], endpoints[ei]);
            // check if result improved
            if (bestColorDistance > indexDistance)
            {
                bestColorDistance = indexDistance;
            }
        }
        error += bestColorDistance;
    }
    return error;
}

constexpr std::size_t ClusterFitMaxIterations = 3;
constexpr float ClusterFitMinDxtError = 0.001F;

// Heuristically fit colors to two color endpoints and their 1/3 and 2/3 intermediate points
// Improves PSNR in the range of 1-2 dB
auto dxtClusterFit(const std::vector<RGBf> &colors, const bool asRGB565) -> std::vector<RGBf>
{
    // calculate initial line fit through RGB color space
    auto endpoints = dxtLineFit(colors, asRGB565);
    auto bestError = calculateError(endpoints, colors, asRGB565);
    // return if the error is already optimal
    if (bestError <= ClusterFitMinDxtError)
    {
        return endpoints;
    }
    // do some rounds of k-means clustering
    std::vector<RGBf> centroids = endpoints;
    for (int iteration = 0; iteration < ClusterFitMaxIterations; ++iteration)
    {
        float iterationError = 0.0F;
        std::vector<std::vector<RGBf>> clusters(4);
        for (const auto &point : colors)
        {
            float minError = std::numeric_limits<float>::max();
            int closestCentroid = -1;
            for (int i = 0; i < 4; ++i)
            {
                auto error = RGBf::mse(point, centroids[i]);
                if (error < minError)
                {
                    minError = error;
                    closestCentroid = i;
                }
            }
            clusters[closestCentroid].push_back(point);
            iterationError += minError;
        }
        // update centroids of cluster c0 and c1 defining the line
        for (int i = 0; i < 2; ++i)
        {
            auto sum = RGBf(0, 0, 0);
            for (const auto &point : clusters[i])
            {
                sum += point;
            }
            centroids[i] = RGBf::roundTo(sum / (double)clusters[i].size(), asRGB565 ? RGB565::Max : XRGB1555::Max);
        }
        // update centroids of intermediate colors c2 and c3
        centroids[2] = RGBf::roundTo(RGBf((centroids[0].cwiseProduct(RGBf(2, 2, 2)) + centroids[1]).cwiseQuotient(RGBf(3, 3, 3))), asRGB565 ? RGB565::Max : XRGB1555::Max);
        centroids[3] = RGBf::roundTo(RGBf((centroids[0] + centroids[1].cwiseProduct(RGBf(2, 2, 2))).cwiseQuotient(RGBf(3, 3, 3))), asRGB565 ? RGB565::Max : XRGB1555::Max);
        // store endpoints if error improved
        if (bestError > iterationError)
        {
            bestError = iterationError;
            endpoints = centroids;
        }
    }
    return endpoints;
}

// This is basically the "range fit" method from here: http://www.sjbrown.co.uk/2006/01/19/dxt-compression-techniques/
auto encodeBlockDXT(const Color::XRGB8888 *start, const uint32_t pixelsPerScanline, const bool asRGB565) -> std::vector<uint8_t>
{
    REQUIRE(pixelsPerScanline % 4 == 0, std::runtime_error, "Image width must be a multiple of 4 for DXT compression");
    // get block colors for all 16 pixels
    std::vector<RGBf> colors(16);
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
#if defined(CLUSTER_FIT)
    auto endpoints = dxtClusterFit(colors, asRGB565);
#else
    auto endpoints = dxtLineFit(colors, asRGB565);
#endif
    // calculate minimum distance for all colors to endpoints to assign indices
    std::vector<uint32_t> endpointIndices(colors.size());
    for (uint32_t ci = 0; ci < 16; ++ci)
    {
        // calculate minimum distance for each index for this color
        float bestColorDistance = std::numeric_limits<float>::max();
        for (uint32_t ei = 0; ei < 4; ++ei)
        {
            auto indexDistance = RGBf::mse(colors[ci], endpoints[ei]);
            // check if result improved
            if (bestColorDistance > indexDistance)
            {
                bestColorDistance = indexDistance;
                endpointIndices[ci] = ei;
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
    for (auto iIt = endpointIndices.crbegin(); iIt != endpointIndices.crend(); ++iIt)
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