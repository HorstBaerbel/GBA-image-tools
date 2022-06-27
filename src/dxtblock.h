#pragma once

#include "color_ycgco.h"
#include "colorhelpers.h"
#include "linefit.h"

#include <Eigen/Core>
#include <Eigen/Dense>
#include <array>
#include <cstdint>
#include <vector>

using namespace Color;

template <std::size_t WIDTH, std::size_t HEIGHT>
class DXTBlock
{
public:
    static constexpr std::size_t Width = WIDTH;
    static constexpr std::size_t Height = HEIGHT;

    DXTBlock() = default;

    DXTBlock(uint16_t color0, uint16_t color1, const std::array<uint8_t, Width * Height> &indices)
        : m_color0(color0), m_color1(color1), m_indices(indices)
    {
    }

    /// @brief Copyies encoded DXT block to binary array
    /// This array can be read in 16-bit chunks
    auto toArray() const -> std::array<uint8_t, 4 + (Width * Height * 2) / 8>
    {
        std::array<uint8_t, 4 + (Width * Height * 2) / 8> result;
        *reinterpret_cast<uint16_t *>(&result[0]) = m_color0;
        *reinterpret_cast<uint16_t *>(&result[2]) = m_color1;
        auto indexPtr = reinterpret_cast<uint16_t *>(&result[4]);
        uint16_t indices16 = 0;
        uint32_t shiftCount = 0;
        for (auto iIt = m_indices.cbegin(); iIt != m_indices.cend(); ++iIt)
        {
            // swap bits around so we end up with the last bit in the highest place
            indices16 = (indices16 >> 2) | (static_cast<uint16_t>(*iIt) << 14);
            if (++shiftCount >= 8)
            {
                *indexPtr++ = indices16;
                indices16 = 0;
                shiftCount = 0;
            }
        }
        return result;
    }

    /// @brief DXT-encodes one NxN block
    /// This is basically the "range fit" method from here: http://www.sjbrown.co.uk/2006/01/19/dxt-compression-techniques/
    static auto encode(const std::array<YCgCoRd, Width * Height> &colors) -> DXTBlock
    {
        // calculate line fit through RGB color space
        auto originAndAxis = lineFit(colors);
        // calculate signed distance from origin
        std::vector<double> distanceFromOrigin(Width * Height);
        std::transform(colors.cbegin(), colors.cend(), distanceFromOrigin.begin(), [origin = originAndAxis.first, axis = originAndAxis.second](const auto &color)
                       { return color.dot(axis); });
        // get the distance of endpoints c0 and c1 on line
        auto minMaxDistance = std::minmax_element(distanceFromOrigin.cbegin(), distanceFromOrigin.cend());
        auto indexC0 = std::distance(distanceFromOrigin.cbegin(), minMaxDistance.first);
        auto indexC1 = std::distance(distanceFromOrigin.cbegin(), minMaxDistance.second);
        // get colors c0 and c1 on line and round to grid
        auto c0 = colors[indexC0];
        auto c1 = colors[indexC1];
        YCgCoRd endpoints[4] = {c0, c1, {}, {}};
        /*if (toPixel(endpoints[0]) > toPixel(endpoints[1]))
        {
            std::swap(c0, c1);
            std::swap(endpoints[0], endpoints[1]);
        }*/
        // calculate intermediate colors c2 and c3 (rounded like in decoder)
        endpoints[2] = YCgCoRd::roundToRGB555((c0.cwiseProduct(YCgCoRd(2, 2, 2)) + c1).cwiseQuotient(YCgCoRd(3, 3, 3)));
        endpoints[3] = YCgCoRd::roundToRGB555((c0 + c1.cwiseProduct(YCgCoRd(2, 2, 2))).cwiseQuotient(YCgCoRd(3, 3, 3)));
        // calculate minimum distance for all colors to endpoints
        std::array<uint8_t, Width *Height> bestIndices = {0};
        for (uint32_t ci = 0; ci < Width * Height; ++ci)
        {
            // calculate minimum distance for each index for this color
            double bestColorDistance = std::numeric_limits<double>::max();
            for (uint32_t ei = 0; ei < 4; ++ei)
            {
                auto indexDistance = YCgCoRd::distance(colors[ci], endpoints[ei]);
                // check if result improved
                if (bestColorDistance > indexDistance)
                {
                    bestColorDistance = indexDistance;
                    bestIndices[ci] = ei;
                }
            }
        }
        return {toBGR555(c0.toRGB555()), toBGR555(c1.toRGB555()), bestIndices};
    }

    static auto decode(const DXTBlock &block) -> std::array<YCgCoRd, Width * Height>
    {
        std::array<YCgCoRd, 4> colors;
        colors[0] = YCgCoRd::fromRGB555(block.m_color0);
        colors[1] = YCgCoRd::fromRGB555(block.m_color1);
        colors[2] = YCgCoRd::roundToRGB555((colors[0].cwiseProduct(YCgCoRd(2, 2, 2)) + colors[1]).cwiseQuotient(YCgCoRd(3, 3, 3)));
        colors[3] = YCgCoRd::roundToRGB555((colors[0] + colors[1].cwiseProduct(YCgCoRd(2, 2, 2))).cwiseQuotient(YCgCoRd(3, 3, 3)));
        uint32_t shift = 0;
        std::array<YCgCoRd, Width * Height> result;
        for (uint32_t i = 0; i < Width * Height; ++i)
        {
            result[i] = colors[(block.m_indices[i] & 0x3)];
        }
        return result;
    }

private:
    uint16_t m_color0 = 0;
    uint16_t m_color1 = 0;
    std::array<uint8_t, Width * Height> m_indices;
};
