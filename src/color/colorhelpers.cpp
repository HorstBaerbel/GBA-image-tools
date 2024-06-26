#include "colorhelpers.h"

#include "exception.h"
#include "processing/datahelpers.h"

#include <algorithm>
#include <cmath>
#include <future>
#include <iomanip>
#include <limits>
#include <numeric>
#include <sstream>

// Redefine QuantumRange here, to avoid an issue with ImageMagick headers
#if (MAGICKCORE_QUANTUM_DEPTH == 8)
#define QuantumRange (255.0)
#elif (MAGICKCORE_QUANTUM_DEPTH == 16)
#define QuantumRange (65535.0)
#elif (MAGICKCORE_QUANTUM_DEPTH == 32)
#define QuantumRange (4294967295.0)
#elif (MAGICKCORE_QUANTUM_DEPTH == 64)
#define QuantumRange (18446744073709551615.0)
#else
#error "Could not define QuantumRange, check the ImageMagick header"
#endif

std::string asHex(const Magick::Color &color)
{
    std::stringstream ss;
    ss << "0x";
    auto colorRGB = Magick::ColorRGB(color);
    ss << std::hex << std::setfill('0') << std::setw(2) << static_cast<uint8_t>(std::round(255.0 * colorRGB.red()));
    ss << std::hex << std::setfill('0') << std::setw(2) << static_cast<uint8_t>(std::round(255.0 * colorRGB.green()));
    ss << std::hex << std::setfill('0') << std::setw(2) << static_cast<uint8_t>(std::round(255.0 * colorRGB.blue()));
    return ss.str();
}

std::vector<Magick::Color> addColorAtIndex0(const std::vector<Magick::Color> &colorMap, const Magick::Color &color0)
{
    std::vector<Magick::Color> tempMap(colorMap.size() + 1, color0);
    std::copy(colorMap.cbegin(), colorMap.cend(), std::next(tempMap.begin()));
    return tempMap;
}

std::vector<std::vector<uint16_t>> convertToBGR555(const std::vector<std::vector<Magick::Color>> &colors)
{
    std::vector<std::vector<uint16_t>> result;
    std::transform(colors.cbegin(), colors.cend(), std::back_inserter(result), ((std::vector<uint16_t>(*)(const std::vector<Magick::Color> &))convertToBGR555));
    return result;
}

std::vector<uint16_t> convertToBGR555(const std::vector<Magick::Color> &colors)
{
    std::vector<uint16_t> result;
    std::transform(colors.cbegin(), colors.cend(), std::back_inserter(result), colorToBGR555);
    return result;
}

uint16_t colorToBGR555(const Magick::Color &color)
{
    auto colorRGB = Magick::ColorRGB(color);
    auto b = static_cast<uint16_t>(std::round(31.0 * colorRGB.blue()));
    auto g = static_cast<uint16_t>(std::round(31.0 * colorRGB.green()));
    auto r = static_cast<uint16_t>(std::round(31.0 * colorRGB.red()));
    return (b << 10 | g << 5 | r);
}

uint16_t toBGR555(uint16_t color)
{
    uint16_t r = (color & 0x7C00) >> 10;
    uint16_t g = (color & 0x3E0);
    uint16_t b = (color & 0x1F);
    return ((b << 10) | g | r);
}

std::vector<uint8_t> toRGB555(const std::vector<uint8_t> &imageData)
{
    std::vector<uint16_t> result;
    // size must be a multiple of 3 for RGB888
    const auto nrOfComponents = imageData.size();
    REQUIRE((nrOfComponents % 3) == 0, std::runtime_error, "Number of components must a multiple of 3");
    for (std::remove_const<decltype(nrOfComponents)>::type i = 0; i < nrOfComponents; i += 3)
    {
        uint16_t r = imageData[i] >> 3;
        uint16_t g = imageData[i + 1] >> 3;
        uint16_t b = imageData[i + 2] >> 3;
        result.push_back((r << 10) | (g << 5) | b);
    }
    return convertTo<uint8_t>(result);
}

uint16_t lerpRGB555(uint16_t c0, uint16_t c1, double t)
{
    if (t <= 0.0)
    {
        return c0;
    }
    else if (t >= 1.0)
    {
        return c1;
    }
    double r0 = (c0 & 0x7C00) >> 10;
    double g0 = (c0 & 0x3E0) >> 5;
    double b0 = c0 & 0x1F;
    double r1 = (c1 & 0x7C00) >> 10;
    double g1 = (c1 & 0x3E0) >> 5;
    double b1 = c1 & 0x1F;
    uint16_t r = std::trunc(r0 + t * (r1 - r0));
    uint16_t g = std::trunc(g0 + t * (g1 - g0));
    uint16_t b = std::trunc(b0 + t * (b1 - b0));
    return (r << 10) | (g << 5) | b;
}

std::array<float, 3> rgb555toArray(uint16_t color)
{
    return {static_cast<float>((color & 0x7C00) >> 10), static_cast<float>((color & 0x3E0) >> 5), static_cast<float>(color & 0x1F)};
}

std::vector<uint16_t> convertToBGR565(const std::vector<Magick::Color> &colors)
{
    std::vector<uint16_t> result;
    std::transform(colors.cbegin(), colors.cend(), std::back_inserter(result), colorToBGR565);
    return result;
}

uint16_t colorToBGR565(const Magick::Color &color)
{
    auto colorRGB = Magick::ColorRGB(color);
    auto b = static_cast<uint16_t>(std::round(31.0 * colorRGB.blue()));
    auto g = static_cast<uint16_t>(std::round(63.0 * colorRGB.green()));
    auto r = static_cast<uint16_t>(std::round(31.0 * colorRGB.red()));
    return (b << 11 | g << 5 | r);
}

uint16_t toBGR565(uint16_t color)
{
    auto r = (color & 0xF800) >> 11;
    auto g = (color & 0x7E0);
    auto b = (color & 0x1F);
    return ((b << 11) | g | r);
}

std::vector<uint8_t> toRGB565(const std::vector<uint8_t> &imageData)
{
    std::vector<uint16_t> result;
    // size must be a multiple of 3 for RGB888
    const auto nrOfComponents = imageData.size();
    REQUIRE((nrOfComponents % 3) == 0, std::runtime_error, "Number of components must a multiple of 3");
    for (std::remove_const<decltype(nrOfComponents)>::type i = 0; i < nrOfComponents; i += 3)
    {
        uint16_t r = imageData[i] >> 3;
        uint16_t g = imageData[i + 1] >> 2;
        uint16_t b = imageData[i + 2] >> 3;
        result.push_back((r << 11) | (g << 5) | b);
    }
    return convertTo<uint8_t>(result);
}

uint16_t lerpRGB565(uint16_t c0, uint16_t c1, double t)
{
    if (t <= 0.0)
    {
        return c0;
    }
    else if (t >= 1.0)
    {
        return c1;
    }
    double r0 = (c0 & 0xF800) >> 11;
    double g0 = (c0 & 0x7E0) >> 5;
    double b0 = c0 & 0x1F;
    double r1 = (c1 & 0xF800) >> 11;
    double g1 = (c1 & 0x7E0) >> 5;
    double b1 = c1 & 0x1F;
    uint16_t r = std::trunc(r0 + t * (r1 - r0));
    uint16_t g = std::trunc(g0 + t * (g1 - g0));
    uint16_t b = std::trunc(b0 + t * (b1 - b0));
    return (r << 11) | (g << 5) | b;
}

std::vector<uint8_t> convertToBGR888(const std::vector<Magick::Color> &colors)
{
    std::vector<uint8_t> result;
    for (decltype(colors.size()) i = 0; i < colors.size(); i++)
    {
        auto colorRGB = Magick::ColorRGB(colors.at(i));
        result.push_back(static_cast<uint8_t>(std::round(255.0 * colorRGB.blue())));
        result.push_back(static_cast<uint8_t>(std::round(255.0 * colorRGB.green())));
        result.push_back(static_cast<uint8_t>(std::round(255.0 * colorRGB.red())));
    }
    return result;
}

Magick::Image buildColorMapRGB555()
{
    std::vector<uint8_t> pixels;
    for (uint8_t r = 0; r < 32; ++r)
    {
        for (uint8_t g = 0; g < 32; ++g)
        {
            for (uint8_t b = 0; b < 32; ++b)
            {
                pixels.push_back((255 * r) / 31);
                pixels.push_back((255 * g) / 31);
                pixels.push_back((255 * b) / 31);
            }
        }
    }
    Magick::Image image(256, 128, "RGB", Magick::StorageType::CharPixel, pixels.data());
    image.type(Magick::ImageType::TrueColorType);
    return image;
}

float distance(const Magick::Color &a, const Magick::Color &b)
{
    if (a == b)
    {
        return 0.0F;
    }
    auto rgbA = Magick::ColorRGB(a);
    auto rgbB = Magick::ColorRGB(b);
    auto ra = rgbA.red();
    auto rb = rgbB.red();
    auto r = 0.5 * (ra + rb);
    auto dR = ra - rb;
    auto dG = rgbA.green() - rgbB.green();
    auto dB = rgbA.blue() - rgbB.blue();
    return sqrt((2.0 + r) * dR * dR + 4.0 * dG * dG + (3.0 - r) * dB * dB);
} // max:  sqrt((2 + 0.5) *  1 *  1 + 4   *  1 *  1 + (3 - 0.5) *  1 *  1) = sqrt(2.5 + 4 + 2.5) = 3

float distanceSqr(const Magick::Color &a, const Magick::Color &b)
{
    if (a == b)
    {
        return 0.0F;
    }
    auto rgbA = Magick::ColorRGB(a);
    auto rgbB = Magick::ColorRGB(b);
    auto ra = rgbA.red();
    auto rb = rgbB.red();
    auto r = 0.5 * (ra + rb);
    auto dR = ra - rb;
    auto dG = rgbA.green() - rgbB.green();
    auto dB = rgbA.blue() - rgbB.blue();
    return (2.0 + r) * dR * dR + 4.0 * dG * dG + (3.0 - r) * dB * dB;
} // max:  (2 + 0.5) *  1 *  1 + 4   *  1 *  1 + (3 - 0.5) *  1 *  1 = 2.5 + 4 + 2.5 = 9

float calculateDistanceRMS(const std::vector<uint8_t> &indices, const std::map<uint8_t, std::vector<float>> &distancesSqrMap)
{
    float sumOfSquares = 0.0F;
    for (uint32_t i = 0; i < (indices.size() - 1); i++)
    {
        sumOfSquares += distancesSqrMap.at(indices.at(i)).at(indices.at(i + 1));
    }
    return std::sqrt(sumOfSquares / indices.size());
}

std::vector<uint8_t> insertIndexOptimal(const std::vector<uint8_t> &indices, const std::map<uint8_t, std::vector<float>> &distancesSqrMap, uint8_t indexToInsert)
{
    float bestDistance = std::numeric_limits<float>::max();
    std::vector<uint8_t> bestOrder = indices;
    // insert index at all possible positions and calculate distance of vector
    for (uint32_t i = 0; i <= indices.size(); i++)
    {
        std::vector<uint8_t> candidate = indices;
        candidate.insert(std::next(candidate.begin(), i), indexToInsert);
        auto candidateDistance = calculateDistanceRMS(candidate, distancesSqrMap);
        if (candidateDistance < bestDistance)
        {
            bestDistance = candidateDistance;
            bestOrder = candidate;
        }
    }
    return bestOrder;
}

std::vector<uint8_t> minimizeColorDistance(const std::vector<Magick::Color> &colors)
{
    // build map with color distance for all possible combinations from palette
    std::map<uint8_t, std::vector<float>> distancesSqrMap;
    for (uint32_t i = 0; i < colors.size(); i++)
    {
        const auto a = colors.at(i);
        std::vector<float> distancesSqr;
        for (const auto &b : colors)
        {
            distancesSqr.push_back(distanceSqr(a, b));
        }
        distancesSqrMap[i] = distancesSqr;
    }
    // sort color indices by hue and lightness first
    std::vector<uint8_t> sortedIndices;
    std::generate_n(std::back_inserter(sortedIndices), colors.size(), [i = 0]() mutable
                    { return i++; });
    constexpr double epsilon = 0.1;
    std::sort(sortedIndices.begin(), sortedIndices.end(), [colors](auto ia, auto ib)
              {
                  auto ca = Magick::ColorHSL(colors.at(ia));
                  auto cb = Magick::ColorHSL(colors.at(ib));
                  auto distH = cb.hue() - ca.hue();
                  auto distS = cb.saturation() - ca.saturation();
                  auto distL = cb.lightness() - ca.lightness();
                  return (distH > epsilon && distS > epsilon && distL > epsilon) ||
                         (std::abs(distH) < epsilon && distS > epsilon && distL > epsilon) ||
                         (std::abs(distH) < epsilon && std::abs(distS) < epsilon && distL > epsilon); });
    // insert colors / indices successively at optimal positions
    std::vector<uint8_t> currentIndices(1, sortedIndices.front());
    for (uint32_t i = 1; i < sortedIndices.size(); i++)
    {
        currentIndices = insertIndexOptimal(currentIndices, distancesSqrMap, sortedIndices.at(i));
    }
    return currentIndices;
}

std::vector<Magick::Color> swapColors(const std::vector<Magick::Color> &colors, const std::vector<uint8_t> &newIndices)
{
    std::vector<Magick::Color> result;
    for (uint32_t i = 0; i < colors.size(); i++)
    {
        result.push_back(colors.at(newIndices[i]));
    }
    return result;
}

std::vector<std::vector<uint8_t>> RGB555DistanceSqrTable()
{
    // build list of colors
    std::vector<Magick::Color> colors;
    for (uint16_t r = 0; r < 32; ++r)
    {
        auto rq = (QuantumRange * static_cast<double>(r)) / 31.0;
        for (uint16_t g = 0; g < 32; ++g)
        {
            auto gq = (QuantumRange * static_cast<double>(g)) / 31.0;
            for (uint16_t b = 0; b < 32; ++b)
            {
                auto bq = (QuantumRange * static_cast<double>(b)) / 31.0;
                colors.push_back(Magick::Color(rq, gq, bq));
            }
        }
    }
    // buld color indices for parallel for
    std::vector<uint16_t> indices(1 << 15);
    std::iota(indices.begin(), indices.end(), 0);
    // calculate distance for colors
    std::vector<std::vector<uint8_t>> result(1 << 15);
#pragma omp parallel for
    for (int i = 0; i < indices.size(); i++)
    {
        const auto ci0 = indices[i];
        const auto c0 = colors[ci0];
        std::vector<uint8_t> distances(1 << 15);
        for (uint32_t ci1 = 0; ci1 < colors.size(); ci1++)
        {
            if (ci0 == ci1)
            {
                // same color, no distance
                distances[ci1] = 0;
            }
            /*else if (ci1 < ci0)
            {
                // color combination already in table, copy
                distances[ci1] = result[ci1][ci0];
            }*/
            else
            {
                // new color combination, calculate distance
                distances[ci1] = std::min(std::round(distanceSqr(c0, colors[ci1]) * 28.333), 255.0);
            }
        }
        result[ci0] = std::move(distances);
    }
    return std::move(result);
}
