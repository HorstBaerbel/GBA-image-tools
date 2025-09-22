#include "dxt.h"

#include "color/colorhelpers.h"
#include "color/conversions.h"
#include "color/rgb565.h"
#include "color/rgbf.h"
#include "color/xrgb1555.h"
#include "color/xrgb8888.h"
#include "if/dxt_tables.h"
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
// This is basically the "range fit" method from here: http://www.sjbrown.co.uk/2006/01/19/dxt-compression-techniques/
auto dxtLineFit(const std::vector<RGBf> &colors, const bool asRGB565) -> std::pair<std::vector<RGBf>, std::vector<RGBf>>
{
    // calculate initial line fit through RGB color space
    auto originAndAxis = lineFit(colors);
    // calculate signed distance along line from origin
    std::vector<float> distanceOnLine(colors.size());
    std::transform(colors.cbegin(), colors.cend(), distanceOnLine.begin(), [axis = originAndAxis.second](const auto &color)
                   { return color.dot(axis); });
    // get the distance of endpoints c0 and c1 on line
    auto minMaxDistance = std::minmax_element(distanceOnLine.cbegin(), distanceOnLine.cend());
    auto i0 = std::distance(distanceOnLine.cbegin(), minMaxDistance.first);
    auto i1 = std::distance(distanceOnLine.cbegin(), minMaxDistance.second);
    // get colors c0 and c1 on line
    std::vector<RGBf> e0(4);
    e0[0] = RGBf::roundTo(colors[i0], asRGB565 ? RGB565::Max : XRGB1555::Max);
    e0[1] = RGBf::roundTo(colors[i1], asRGB565 ? RGB565::Max : XRGB1555::Max);
    // calculate intermediate colors c2 and c3 at 1/3 and 2/3
    e0[2] = RGBf::roundTo(RGBf((e0[0].cwiseProduct(RGBf(2, 2, 2)) + e0[1]).cwiseQuotient(RGBf(3, 3, 3))), asRGB565 ? RGB565::Max : XRGB1555::Max);
    e0[3] = RGBf::roundTo(RGBf((e0[0] + e0[1].cwiseProduct(RGBf(2, 2, 2))).cwiseQuotient(RGBf(3, 3, 3))), asRGB565 ? RGB565::Max : XRGB1555::Max);
    // get colors c0 and c1 on line
    std::vector<RGBf> e1(4);
    e1[0] = e0[0];
    e1[1] = e0[1];
    // calculate intermediate color c3 at 1/2 and add black
    e1[2] = RGBf::roundTo(RGBf((e1[0] + e1[1]).cwiseQuotient(RGBf(2, 2, 2))), asRGB565 ? RGB565::Max : XRGB1555::Max);
    e1[3] = RGBf(0, 0, 0);
    return {e0, e1};
}

auto calculateError(const std::vector<RGBf> &endpoints, const std::vector<RGBf> &colors) -> float
{
    // calculate minimum distance for all colors to endpoints and calculate error to that endpoint
    float error = 0.0F;
    for (uint32_t ci = 0; ci < colors.size(); ++ci)
    {
        // calculate minimum distance for each index for this color
        float bestColorDistance = std::numeric_limits<float>::max();
        for (uint32_t ei = 0; ei < endpoints.size(); ++ei)
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

constexpr std::size_t ClusterFitMaxIterations = 3; // 3 seems to be more or less the optimum. only marginal improvements with more iterations
constexpr float ClusterFitMinDxtError = 0.01F;
constexpr float LineFitMinC0C1Error = 0.01F;

// Heuristically fit colors to two color endpoints and their 1/3 and 2/3 or 1/2 intermediate points
// Improves PSNR about 1-2 dB
auto dxtClusterFit(const std::vector<RGBf> &colors, const bool asRGB565) -> std::pair<std::vector<RGBf>, bool>
{
    // calculate initial line fit through RGB color space
    auto guess = dxtLineFit(colors, asRGB565);
    // return if the colors are already VERY close together
    if (RGBf::mse(guess.second[0], guess.second[1]) <= LineFitMinC0C1Error)
    {
        return {guess.second, false};
    }
    auto bestErrorThird = calculateError(guess.first, colors);
    auto bestErrorHalf = calculateError(guess.second, colors);
    bool isModeThird = bestErrorThird < bestErrorHalf;
    std::vector<RGBf> endpoints = isModeThird ? guess.first : guess.second;
    auto bestError = isModeThird ? bestErrorThird : bestErrorHalf;
    // return if the error is already optimal
    if (bestError <= ClusterFitMinDxtError)
    {
        return {endpoints, isModeThird};
    }
    // do some rounds of k-means clustering for 1/3, 2/3 mode, then 1/2 mode
    for (int mode = 0; mode < 2; ++mode)
    {
        std::vector<RGBf> centroids = mode == 0 ? guess.first : guess.second;
        for (int iteration = 0; iteration < ClusterFitMaxIterations; ++iteration)
        {
            float iterationError = 0.0F;
            std::vector<std::vector<RGBf>> clusters(centroids.size());
            for (const auto &point : colors)
            {
                float minError = std::numeric_limits<float>::max();
                int closestCentroid = 0;
                for (int i = 0; i < centroids.size(); ++i)
                {
                    auto error = RGBf::mse(point, centroids[i]);
                    if (error < minError)
                    {
                        minError = error;
                        closestCentroid = i;
                    }
                }
                REQUIRE(closestCentroid >= 0 && closestCentroid < clusters.size(), std::runtime_error, "Bad cluster index");
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
            if (mode == 0)
            {
                // update centroids of intermediate colors c2 and c3 at 1/3 and 2/3
                centroids[2] = RGBf::roundTo(RGBf((centroids[0].cwiseProduct(RGBf(2, 2, 2)) + centroids[1]).cwiseQuotient(RGBf(3, 3, 3))), asRGB565 ? RGB565::Max : XRGB1555::Max);
                centroids[3] = RGBf::roundTo(RGBf((centroids[0] + centroids[1].cwiseProduct(RGBf(2, 2, 2))).cwiseQuotient(RGBf(3, 3, 3))), asRGB565 ? RGB565::Max : XRGB1555::Max);
            }
            else
            {
                // update centroid of intermediate color c2 at 1/2 and add black
                centroids[2] = RGBf::roundTo(RGBf((centroids[0] + centroids[1]).cwiseQuotient(RGBf(2, 2, 2))), asRGB565 ? RGB565::Max : XRGB1555::Max);
                centroids[3] = RGBf(0, 0, 0);
            }
            // store endpoints if error improved
            if (bestError > iterationError)
            {
                bestError = iterationError;
                endpoints = centroids;
                isModeThird = mode == 0;
            }
        }
    }
    return {endpoints, isModeThird};
}

// ------------------------------------------------------------------------------------------------

template <unsigned BLOCK_DIM>
auto encodeBlockInternal(const XRGB8888 *blockStart, const uint32_t pixelsPerScanline, const bool asRGB565, const bool swapToBGR) -> std::vector<uint8_t>
{
    REQUIRE(pixelsPerScanline % BLOCK_DIM == 0, std::runtime_error, "Image width must be a multiple of " << BLOCK_DIM << " for DXT compression");
    // get block colors for all pixels
    constexpr unsigned NrOfPixels = BLOCK_DIM * BLOCK_DIM;
    std::vector<RGBf> colors(NrOfPixels);
    auto cIt = colors.begin();
    auto pixels = blockStart;
    for (int y = 0; y < BLOCK_DIM; y++)
    {
        for (int x = 0; x < BLOCK_DIM; x++)
        {
            *cIt++ = convertTo<RGBf>(pixels[x]);
        }
        pixels += pixelsPerScanline;
    }
#if defined(CLUSTER_FIT)
    auto [endpoints, isModeThird] = dxtClusterFit(colors, asRGB565);
#else
    auto guess = dxtLineFit(colors, asRGB565);
    bool isModeThird;
    std::vector<RGBf> endpoints;
    if (RGBf::mse(guess.second[0], guess.second[1]) <= LineFitMinC0C1Error)
    {
        // if colors are almost identical, use 1/2 mode and second set of endpoints
        endpoints = guess.second;
        isModeThird = false;
    }
    else
    {
        // check if 1/2, 2/3 or 1/2 mode gives lower error
        auto bestErrorThird = calculateError(guess.first, colors);
        auto bestErrorHalf = calculateError(guess.second, colors);
        isModeThird = bestErrorThird < bestErrorHalf;
        endpoints = isModeThird ? guess.first : guess.second;
    }
#endif
    // calculate minimum distance for all colors to endpoints to assign indices
    std::vector<uint32_t> endpointIndices(NrOfPixels);
    for (uint32_t ci = 0; ci < NrOfPixels; ++ci)
    {
        // calculate minimum distance for each index for this color
        float bestColorDistance = std::numeric_limits<float>::max();
        for (uint32_t ei = 0; ei < endpoints.size(); ++ei)
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
    // swap to BGR here if needed. we need to do this AFTER assigning indices
    if (swapToBGR)
    {
        endpoints[0] = endpoints[0].swapToBGR();
        endpoints[1] = endpoints[1].swapToBGR();
    }
    // check how we need to encode the endpoint colors
    auto c0 = asRGB565 ? static_cast<uint16_t>(convertTo<RGB565>(endpoints[0])) : static_cast<uint16_t>(convertTo<XRGB1555>(endpoints[0]));
    auto c1 = asRGB565 ? static_cast<uint16_t>(convertTo<RGB565>(endpoints[1])) : static_cast<uint16_t>(convertTo<XRGB1555>(endpoints[1]));
    if (isModeThird)
    {
        // if we're using 1/3, 2/3 intermediates, store so that always c0 > c1. c0 != c1 here due to being checked above
        if (c0 < c1)
        {
            std::swap(c0, c1);
            for (auto &index : endpointIndices)
            {
                index = index == 0 ? 1 : (index == 1 ? 0 : index);
                index = index == 2 ? 3 : (index == 3 ? 2 : index);
            }
        }
    }
    else
    {
        // if we're using a 1/2 intermediate, store so that always c0 <= c1
        if (c0 > c1)
        {
            std::swap(c0, c1);
            for (auto &index : endpointIndices)
            {
                index = index == 0 ? 1 : (index == 1 ? 0 : index);
            }
        }
    }
    // build result data
    std::vector<uint8_t> result(2 * 2 + NrOfPixels * 2 / 8);
    // add color endpoints c0 and c1
    auto data16 = reinterpret_cast<uint16_t *>(result.data());
    *data16++ = c0;
    *data16++ = c1;
    // add index data making sure the lower indices are in the lower bits
    uint16_t indices = 0;
    for (auto iIt = endpointIndices.cbegin(); iIt != endpointIndices.cend(); ++iIt)
    {
        indices = (indices >> 2) | (*iIt << 14);
        if ((std::distance(endpointIndices.cbegin(), iIt) + 1) % 8 == 0)
        {
            *data16++ = indices;
            indices = 0;
        }
    }
    return result;
}

template <>
auto DXT::encodeBlock<4>(const std::vector<Color::XRGB8888> &block, const bool asRGB565, const bool swapToBGR) -> std::vector<uint8_t>
{
    REQUIRE(block.size() == 16, std::runtime_error, "Number of pixels in block must be 16");
    return encodeBlockInternal<4>(block.data(), 4, asRGB565, swapToBGR);
}

template <>
auto DXT::encodeBlock<8>(const std::vector<Color::XRGB8888> &block, const bool asRGB565, const bool swapToBGR) -> std::vector<uint8_t>
{
    REQUIRE(block.size() == 64, std::runtime_error, "Number of pixels in block must be 64");
    return encodeBlockInternal<8>(block.data(), 8, asRGB565, swapToBGR);
}

template <>
auto DXT::encodeBlock<16>(const std::vector<Color::XRGB8888> &block, const bool asRGB565, const bool swapToBGR) -> std::vector<uint8_t>
{
    REQUIRE(block.size() == 256, std::runtime_error, "Number of pixels in block must be 256");
    return encodeBlockInternal<16>(block.data(), 16, asRGB565, swapToBGR);
}

auto DXT::encode(const std::vector<XRGB8888> &image, const uint32_t width, const uint32_t height, const bool asRGB565, const bool swapToBGR) -> std::vector<uint8_t>
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
            auto block = encodeBlockInternal<4>(image.data() + y * width + x, width, asRGB565, swapToBGR);
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

// ------------------------------------------------------------------------------------------------

template <unsigned BLOCK_DIM>
auto decodeBlockInternal(const uint16_t *colorStart, const uint16_t *indexStart, Color::XRGB8888 *blockStart, const uint32_t pixelsPerScanline, const bool asRGB565, const bool swapToBGR) -> void
{
    // read colors c0 and c1
    std::array<XRGB8888, 4> colors;
    uint16_t c0 = *colorStart++;
    uint16_t c1 = *colorStart++;
    const bool isModeThird = c0 > c1;
    if (swapToBGR)
    {
        c0 = asRGB565 ? static_cast<uint16_t>(RGB565(c0).swapToBGR()) : static_cast<uint16_t>(XRGB1555(c0).swapToBGR());
        c1 = asRGB565 ? static_cast<uint16_t>(RGB565(c1).swapToBGR()) : static_cast<uint16_t>(XRGB1555(c1).swapToBGR());
    }
    colors[0] = asRGB565 ? convertTo<XRGB8888>(RGB565(c0)) : convertTo<XRGB8888>(XRGB1555(c0));
    colors[1] = asRGB565 ? convertTo<XRGB8888>(RGB565(c1)) : convertTo<XRGB8888>(XRGB1555(c1));
    // check if c0 > c1 -> 1/3, 2/3 mode, or if c0 <= c1 -> 1/2 mode
    if (isModeThird)
    {
        // calculate intermediate colors c2 at 1/3 and c3 at 2/3 using tables
        uint32_t c2c3;
        if (asRGB565)
        {
            uint32_t b = ((c0 & 0xF800) >> 6) | ((c1 & 0xF800) >> 11);
            uint32_t g = ((c0 & 0x7E0) << 1) | ((c1 & 0x7E0) >> 5);
            uint32_t r = ((c0 & 0x1F) << 5) | (c1 & 0x1F);
            c2c3 = (DXT_C2C3_ModeThird_5bit[b] << 11) | (DXT_C2C3_ModeThird_6bit[g] << 5) | DXT_C2C3_ModeThird_5bit[r];
        }
        else
        {
            uint32_t b = ((c0 & 0x7C00) >> 5) | ((c1 & 0x7C00) >> 10);
            uint32_t g = (c0 & 0x3E0) | ((c1 & 0x3E0) >> 5);
            uint32_t r = ((c0 & 0x1F) << 5) | (c1 & 0x1F);
            c2c3 = (DXT_C2C3_ModeThird_5bit[b] << 10) | (DXT_C2C3_ModeThird_5bit[g] << 5) | DXT_C2C3_ModeThird_5bit[r];
        }
        uint16_t c2 = (c2c3 & 0x0000FFFF);
        uint16_t c3 = ((c2c3 & 0xFFFF0000) >> 16);
        colors[2] = asRGB565 ? convertTo<XRGB8888>(RGB565(c2)) : convertTo<XRGB8888>(XRGB1555(c2));
        colors[3] = asRGB565 ? convertTo<XRGB8888>(RGB565(c3)) : convertTo<XRGB8888>(XRGB1555(c3));
    }
    else
    {
        // calculate intermediate color c2 at 1/2 and leave c3 at black
        uint16_t c2;
        if (asRGB565)
        {
            uint32_t b = (((c0 & 0xF800) >> 11) + ((c1 & 0xF800) >> 11) + 1) >> 1;
            uint32_t g = (((c0 & 0x7E0) >> 5) + ((c1 & 0x7E0) >> 5) + 1) >> 1;
            uint32_t r = ((c0 & 0x1F) + (c1 & 0x1F) + 1) >> 1;
            c2 = ((b << 11) | (g << 5) | r);
        }
        else
        {
            uint32_t b = (((c0 & 0x7C00) >> 10) + ((c1 & 0x7C00) >> 10) + 1) >> 1;
            uint32_t g = (((c0 & 0x3E0) >> 5) + ((c1 & 0x3E0) >> 5) + 1) >> 1;
            uint32_t r = ((c0 & 0x1F) + (c1 & 0x1F) + 1) >> 1;
            c2 = ((b << 10) | (g << 5) | r);
        }
        colors[2] = asRGB565 ? convertTo<XRGB8888>(RGB565(c2)) : convertTo<XRGB8888>(XRGB1555(c2));
        colors[3] = XRGB8888(0, 0, 0);
    }
    // and decode colors
    uint16_t indices = 0;
    auto pixel = blockStart;
    for (std::size_t y = 0; y < BLOCK_DIM; y++)
    {
        // read pixel color indices
        if ((y * BLOCK_DIM) % 8 == 0)
        {
            indices = *indexStart++;
        }
        for (std::size_t x = 0; x < BLOCK_DIM; x++)
        {
            pixel[x] = colors[indices & 0x03];
            indices >>= 2;
        }
        pixel += pixelsPerScanline;
    }
}

template <>
auto DXT::decodeBlock<4>(const std::vector<uint8_t> &data, const bool asRGB565, const bool swapToBGR) -> std::vector<Color::XRGB8888>
{
    REQUIRE(data.size() == 2 + 2 + 16 * 2 / 8, std::runtime_error, "Data size must be 8");
    std::vector<Color::XRGB8888> block(16);
    decodeBlockInternal<4>(reinterpret_cast<const uint16_t *>(data.data()), reinterpret_cast<const uint16_t *>(data.data() + 4), block.data(), 4, asRGB565, swapToBGR);
    return block;
}

template <>
auto DXT::decodeBlock<8>(const std::vector<uint8_t> &data, const bool asRGB565, const bool swapToBGR) -> std::vector<Color::XRGB8888>
{
    REQUIRE(data.size() == 2 + 2 + 64 * 2 / 8, std::runtime_error, "Data size must be 20");
    std::vector<Color::XRGB8888> block(64);
    decodeBlockInternal<8>(reinterpret_cast<const uint16_t *>(data.data()), reinterpret_cast<const uint16_t *>(data.data() + 4), block.data(), 8, asRGB565, swapToBGR);
    return block;
}

template <>
auto DXT::decodeBlock<16>(const std::vector<uint8_t> &data, const bool asRGB565, const bool swapToBGR) -> std::vector<Color::XRGB8888>
{
    REQUIRE(data.size() == 2 + 2 + 256 * 2 / 8, std::runtime_error, "Data size must be 68");
    std::vector<Color::XRGB8888> block(256);
    decodeBlockInternal<16>(reinterpret_cast<const uint16_t *>(data.data()), reinterpret_cast<const uint16_t *>(data.data() + 4), block.data(), 16, asRGB565, swapToBGR);
    return block;
}

auto DXT::decode(const std::vector<uint8_t> &data, const uint32_t width, const uint32_t height, const bool asRGB565, const bool swapToBGR) -> std::vector<XRGB8888>
{
    REQUIRE(width % 4 == 0, std::runtime_error, "Image width must be a multiple of 4 for DXT decompression");
    REQUIRE(height % 4 == 0, std::runtime_error, "Image height must be a multiple of 4 for DXT decompression");
    const auto nrOfBlocks = data.size() / 8;
    REQUIRE(nrOfBlocks == width / 4 * height / 4, std::runtime_error, "Data size does not match image size");
    std::vector<XRGB8888> result(width * height);
#pragma omp parallel for
    for (std::size_t y = 0; y < height; y += 4)
    {
        // set up pointers to source block data
        auto blockIndex = y / 4 * width / 4;
        auto color16 = reinterpret_cast<const uint16_t *>(data.data() + blockIndex * 4);
        auto indices16 = reinterpret_cast<const uint16_t *>(data.data() + nrOfBlocks * 4 + blockIndex * 4);
        auto blockStart = result.data() + y * width;
        for (std::size_t x = 0; x < width; x += 4)
        {
            decodeBlockInternal<4>(color16, indices16, blockStart, width, asRGB565, swapToBGR);
            color16 += 2;
            indices16 += 2;
            blockStart += 4;
        }
    }
    return result;
}