#include "colorhelpers.h"

#include <algorithm>
#include <cmath>
#include <execution>
#include <future>
#include <iomanip>
#include <limits>
#include <numeric>
#include <sstream>

std::string asHex(const Magick::Color &color)
{
    std::stringstream ss;
    ss << "0x";
    ss << std::hex << std::setfill('0') << std::setw(2) << static_cast<uint8_t>(std::round(255.0 * Magick::Color::scaleQuantumToDouble(color.redQuantum())));
    ss << std::hex << std::setfill('0') << std::setw(2) << static_cast<uint8_t>(std::round(255.0 * Magick::Color::scaleQuantumToDouble(color.greenQuantum())));
    ss << std::hex << std::setfill('0') << std::setw(2) << static_cast<uint8_t>(std::round(255.0 * Magick::Color::scaleQuantumToDouble(color.blueQuantum())));
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
    auto b = static_cast<uint16_t>(std::round(31.0 * Magick::Color::scaleQuantumToDouble(color.blueQuantum())));
    auto g = static_cast<uint16_t>(std::round(31.0 * Magick::Color::scaleQuantumToDouble(color.greenQuantum())));
    auto r = static_cast<uint16_t>(std::round(31.0 * Magick::Color::scaleQuantumToDouble(color.redQuantum())));
    return (b << 10 | g << 5 | r);
}

uint16_t toBGR555(uint16_t color)
{
    auto r = (color & 0x7C00) >> 10;
    auto g = (color & 0x3E0);
    auto b = (color & 0x1F);
    return ((b << 10) | g | r);
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

std::vector<uint16_t> convertToBGR565(const std::vector<Magick::Color> &colors)
{
    std::vector<uint16_t> result;
    std::transform(colors.cbegin(), colors.cend(), std::back_inserter(result), colorToBGR565);
    return result;
}

uint16_t colorToBGR565(const Magick::Color &color)
{
    auto b = static_cast<uint16_t>(std::round(31.0 * Magick::Color::scaleQuantumToDouble(color.blueQuantum())));
    auto g = static_cast<uint16_t>(std::round(63.0 * Magick::Color::scaleQuantumToDouble(color.greenQuantum())));
    auto r = static_cast<uint16_t>(std::round(31.0 * Magick::Color::scaleQuantumToDouble(color.redQuantum())));
    return (b << 11 | g << 5 | r);
}

uint16_t toBGR565(uint16_t color)
{
    auto r = (color & 0xF800) >> 11;
    auto g = (color & 0x7E0);
    auto b = (color & 0x1F);
    return ((b << 11) | g | r);
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
        const auto &color = colors.at(i);
        result.push_back(static_cast<uint8_t>(std::round(255.0 * Magick::Color::scaleQuantumToDouble(color.blueQuantum()))));
        result.push_back(static_cast<uint8_t>(std::round(255.0 * Magick::Color::scaleQuantumToDouble(color.greenQuantum()))));
        result.push_back(static_cast<uint8_t>(std::round(255.0 * Magick::Color::scaleQuantumToDouble(color.redQuantum()))));
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
    auto ra = Magick::Color::scaleQuantumToDouble(a.redQuantum());
    auto rb = Magick::Color::scaleQuantumToDouble(b.redQuantum());
    auto r = 0.5 * (ra + rb);
    auto dR = ra - rb;
    auto dG = Magick::Color::scaleQuantumToDouble(a.greenQuantum()) - Magick::Color::scaleQuantumToDouble(b.greenQuantum());
    auto dB = Magick::Color::scaleQuantumToDouble(a.blueQuantum()) - Magick::Color::scaleQuantumToDouble(b.blueQuantum());
    return sqrt((2.0 + r) * dR * dR + 4.0 * dG * dG + (3.0 - r) * dB * dB);
} // max:  sqrt((2   + 1) *  1 *  1 + 4   *  1 *  1 + (3   - 1) *  1 *  1) = sqrt(3 + 4 + 2) = 3

float distanceSqr(const Magick::Color &a, const Magick::Color &b)
{
    if (a == b)
    {
        return 0.0F;
    }
    auto ra = Magick::Color::scaleQuantumToDouble(a.redQuantum());
    auto rb = Magick::Color::scaleQuantumToDouble(b.redQuantum());
    auto r = 0.5 * (ra + rb);
    auto dR = ra - rb;
    auto dG = Magick::Color::scaleQuantumToDouble(a.greenQuantum()) - Magick::Color::scaleQuantumToDouble(b.greenQuantum());
    auto dB = Magick::Color::scaleQuantumToDouble(a.blueQuantum()) - Magick::Color::scaleQuantumToDouble(b.blueQuantum());
    return (2.0 + r) * dR * dR + 4.0 * dG * dG + (3.0 - r) * dB * dB;
} // max:  (2   + 1) *  1 *  1 + 4   *  1 *  1 + (3   - 1) *  1 *  1 = 3 + 4 + 2 = 9

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
                  auto distI = cb.intensity() - ca.intensity();
                  auto distL = cb.luminosity() - ca.luminosity();
                  return (distH > epsilon && distI > epsilon && distL > epsilon) ||
                         (std::abs(distH) < epsilon && distI > epsilon && distL > epsilon) ||
                         (std::abs(distH) < epsilon && std::abs(distI) < epsilon && distL > epsilon); });
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
        auto rq = Magick::Color::scaleDoubleToQuantum(static_cast<double>(r) / 31.0);
        for (uint16_t g = 0; g < 32; ++g)
        {
            auto gq = Magick::Color::scaleDoubleToQuantum(static_cast<double>(g) / 31.0);
            for (uint16_t b = 0; b < 32; ++b)
            {
                auto bq = Magick::Color::scaleDoubleToQuantum(static_cast<double>(b) / 31.0);
                colors.push_back(Magick::Color(rq, gq, bq));
            }
        }
    }
    // buld color indices for parallel for
    std::vector<uint16_t> indices(1 << 15);
    std::iota(indices.begin(), indices.end(), 0);
    // calculate distance for colors
    std::vector<std::vector<uint8_t>> result(1 << 15);
    std::for_each(std::execution::par_unseq, indices.cbegin(), indices.cend(), [&](uint16_t ci0)
                  {
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
                      result[ci0] = std::move(distances); });
    return std::move(result);
}
