#include "colorhelpers.h"

#include <limits>
#include <algorithm>
#include <numeric>
#include <cmath>
#include <future>
#include <sstream>
#include <iomanip>

std::vector<Magick::Color> getColorMap(const Magick::Image &img)
{
    std::vector<Magick::Color> colorMap(img.colorMapSize());
    for (std::remove_const<decltype(colorMap.size())>::type i = 0; i < colorMap.size(); ++i)
    {
        colorMap[i] = img.colorMap(i);
    }
    return colorMap;
}

void setColorMap(Magick::Image &img, const std::vector<Magick::Color> &colorMap)
{
    for (std::remove_const<decltype(colorMap.size())>::type i = 0; i < colorMap.size(); ++i)
    {
        img.colorMap(i, colorMap.at(i));
    }
}

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

std::vector<Magick::Color> interleave(const std::vector<std::vector<Magick::Color>> &palettes)
{
    // make sure all vectors have the same sizes
    for (const auto &p : palettes)
    {
        if (p.size() != palettes.front().size())
        {
            throw std::runtime_error("All palettes must have the same number of colors!");
        }
    }
    std::vector<Magick::Color> result;
    for (uint32_t ci = 0; ci < palettes.front().size(); ci++)
    {
        for (const auto &p : palettes)
        {
            result.push_back(p[ci]);
        }
    }
    return result;
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
}

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
            auto distAB = distance(a, b);
            distancesSqr.push_back(distAB * distAB);
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
                         (std::abs(distH) < epsilon && std::abs(distI) < epsilon && distL > epsilon);
              });
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
