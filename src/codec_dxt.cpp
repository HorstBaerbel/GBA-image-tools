#include "codec_dxt.h"

#include "colorhelpers.h"
#include "exception.h"

#include <array>
#include <execution>

std::vector<std::vector<uint8_t>> DXT::RGB555DistanceSqrCache;

std::vector<uint8_t> DXT::encodeBlockDXTG(const uint16_t *start, uint32_t pixelsPerScanline, const std::vector<std::vector<uint8_t>> &distanceSqrMap)
{
    REQUIRE(pixelsPerScanline % 4 == 0, std::runtime_error, "Image width must be a multiple of 4 for DXT compression");
    // get block colors for all 16 pixels
    std::array<uint16_t, 16> colors = {0};
    auto c16It = colors.begin();
    auto pixel = start;
    for (int y = 0; y < 4; y++)
    {
        *c16It++ = pixel[0];
        *c16It++ = pixel[1];
        *c16It++ = pixel[2];
        *c16It++ = pixel[3];
        pixel += pixelsPerScanline;
    }
    // calculate minimum color distance for each combination of block endpoints
    float bestIterationDistance = std::numeric_limits<int32_t>::max();
    uint16_t bestC0 = colors.front();
    uint16_t bestC1 = colors.front();
    std::array<uint32_t, 16> bestIndices = {0};
    std::array<uint32_t, 16> iterationIndices;
    for (auto c0It = colors.cbegin(); c0It != colors.cend(); ++c0It)
    {
        uint16_t endpoints[4] = {*c0It, 0, 0, 0};
        for (auto c1It = colors.cbegin(); c1It != colors.cend(); ++c1It)
        {
            endpoints[1] = *c1It;
            if (*c0It > *c1It)
            {
                std::swap(endpoints[0], endpoints[1]);
            }
            // calculate intermediate colors c2 and c3
            endpoints[2] = lerpRGB555(*c0It, *c1It, 1.0 / 3.0);
            endpoints[3] = lerpRGB555(*c0It, *c1It, 2.0 / 3.0);
            // calculate minimum distance for all colors to endpoints
            float iterationDistance = 0;
            std::fill(iterationIndices.begin(), iterationIndices.end(), 0);
            for (uint32_t ci = 0; ci < 16; ++ci)
            {
                const auto &distMapColorI = distanceSqrMap[colors[ci]];
                // calculate minimum distance for each index for this color
                uint8_t bestColorDistance = std::numeric_limits<uint8_t>::max();
                for (uint32_t ei = 0; ei < 4; ++ei)
                {
                    auto indexDistance = distMapColorI[endpoints[ei]];
                    // check if result improved
                    if (bestColorDistance > indexDistance)
                    {
                        bestColorDistance = indexDistance;
                        iterationIndices[ci] = ei;
                    }
                }
                iterationDistance += bestColorDistance;
            }
            // check if result improved
            iterationDistance = std::sqrt(iterationDistance / 16);
            if (bestIterationDistance > iterationDistance)
            {
                bestIterationDistance = iterationDistance;
                bestC0 = endpoints[0];
                bestC1 = endpoints[1];
                bestIndices = iterationIndices;
            }
        }
    }
    // build result data
    std::vector<uint8_t> result(2 * 2 + 16 * 2 / 8);
    // add color endpoints c0 and c1
    auto data16 = reinterpret_cast<uint16_t *>(result.data());
    *data16++ = toBGR555(bestC0);
    *data16++ = toBGR555(bestC1);
    // add index data in reverse
    uint32_t indices = 0;
    for (auto iIt = bestIndices.crbegin(); iIt != bestIndices.crend(); ++iIt)
    {
        indices = (indices << 2) | *iIt;
    }
    *(reinterpret_cast<uint32_t *>(data16)) = indices;
    return result;
}

auto DXT::encodeDXTG(const std::vector<uint16_t> &image, uint32_t width, uint32_t height) -> std::vector<uint8_t>
{
    REQUIRE(width % 4 == 0, std::runtime_error, "Image width must be a multiple of 4 for DXT compression");
    REQUIRE(height % 4 == 0, std::runtime_error, "Image height must be a multiple of 4 for DXT compression");
    // check if squared distance map has been allocated
    if (RGB555DistanceSqrCache.empty())
    {
        RGB555DistanceSqrCache = std::move(RGB555DistanceSqrTable());
    }
    // build y-position table for parallel for
    std::vector<uint32_t> ys(height / 4);
    std::generate(ys.begin(), ys.end(), [y = 0]() mutable
                  {
                          y += 4;
                          return y - 4; });
    // compress to DXT1. we get 8 bytes per 4x4 block / 16 pixels
    const auto yStride = width * 8 / 16;
    const auto nrOfBlocks = width / 4 * height / 4;
    std::vector<uint8_t> resultData(nrOfBlocks * 8);
    std::for_each(std::execution::par_unseq, ys.cbegin(), ys.cend(), [&](uint32_t y)
                  {
                          for (uint32_t x = 0; x < width; x += 4)
                          {
                              auto block = encodeBlockDXTG(image.data() + y * width + x, width, RGB555DistanceSqrCache);
                              std::copy(block.cbegin(), block.cend(), std::next(resultData.begin(), y * yStride + x * 8 / 4));
                          } });
    // split data into colors and indices for better compression
    std::vector<uint8_t> splitData(nrOfBlocks * 8);
    auto srcPtr = reinterpret_cast<const uint16_t *>(resultData.data());
    auto colorPtr = reinterpret_cast<uint16_t *>(splitData.data());
    auto indexPtr = reinterpret_cast<uint16_t *>(splitData.data() + nrOfBlocks * 8 / 2);
    for (uint32_t i = 0; i < nrOfBlocks; i++)
    {
        *colorPtr++ = *srcPtr++;
        *colorPtr++ = *srcPtr++;
        *indexPtr++ = *srcPtr++;
        *indexPtr++ = *srcPtr++;
    }
    return splitData;
}

auto decodeDXTG(const std::vector<uint8_t> &data, uint32_t width, uint32_t height) -> std::vector<uint8_t>
{
    return {};
}