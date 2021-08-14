#include "colorhelpers.h"

#include <limits>
#include <algorithm>
#include <numeric>
#include <cmath>
#include <future>
#include <iostream>
#include <sstream>

std::vector<Color> getColorMap(const Image &img)
{
    std::vector<Color> colorMap(img.colorMapSize());
    for (std::remove_const<decltype(colorMap.size())>::type i = 0; i < colorMap.size(); ++i)
    {
        colorMap[i] = img.colorMap(i);
    }
    return colorMap;
}

void setColorMap(Image &img, const std::vector<Color> &colorMap)
{
    for (std::remove_const<decltype(colorMap.size())>::type i = 0; i < colorMap.size(); ++i)
    {
        img.colorMap(i, colorMap.at(i));
    }
}

std::string asHex(const Color &color)
{
    std::stringstream ss;
    ss << "0x";
    ss << std::hex << static_cast<uint32_t>(color.redQuantum()) / static_cast<uint32_t>(QuantumRange);
    ss << std::hex << static_cast<uint32_t>(color.greenQuantum()) / static_cast<uint32_t>(QuantumRange);
    ss << std::hex << static_cast<uint32_t>(color.blueQuantum()) / static_cast<uint32_t>(QuantumRange);
    return ss.str();
}

std::vector<Color> addColorAtIndex0(const std::vector<Color> &colorMap, const Color &color0)
{
    std::vector<Color> tempMap(colorMap.size() + 1, color0);
    std::copy(colorMap.cbegin(), colorMap.cend(), std::next(tempMap.begin()));
    return tempMap;
}

std::vector<std::vector<uint16_t>> convertToBGR555(const std::vector<std::vector<Color>> &colors)
{
    std::vector<std::vector<uint16_t>> result;
    std::transform(colors.cbegin(), colors.cend(), std::back_inserter(result), ((std::vector<uint16_t>(*)(const std::vector<Color> &))convertToBGR555));
    return result;
}

std::vector<uint16_t> convertToBGR555(const std::vector<Color> &colors)
{
    std::vector<uint16_t> result;
    std::transform(colors.cbegin(), colors.cend(), std::back_inserter(result), colorToBGR555);
    return result;
}

uint16_t colorToBGR555(const Color &color)
{
    uint16_t b = static_cast<uint16_t>((31 * static_cast<uint32_t>(color.blueQuantum())) / static_cast<uint32_t>(QuantumRange));
    uint16_t g = static_cast<uint16_t>((31 * static_cast<uint32_t>(color.greenQuantum())) / static_cast<uint32_t>(QuantumRange));
    uint16_t r = static_cast<uint16_t>((31 * static_cast<uint32_t>(color.redQuantum())) / static_cast<uint32_t>(QuantumRange));
    return (b << 10 | g << 5 | r);
}

std::vector<Color> interleave(const std::vector<std::vector<Color>> &palettes)
{
    // make sure all vectors have the same sizes
    for (const auto &p : palettes)
    {
        if (p.size() != palettes.front().size())
        {
            throw std::runtime_error("All palettes must have the same number of colors!");
        }
    }
    std::vector<Color> result;
    for (uint32_t ci = 0; ci < palettes.front().size(); ci++)
    {
        for (const auto &p : palettes)
        {
            result.push_back(p[ci]);
        }
    }
    return result;
}

float distance(const Color &a, const Color &b)
{
    if (a == b)
    {
        return 0.0F;
    }
    float ra = (float)a.redQuantum() / (float)QuantumRange;
    float rb = (float)b.redQuantum() / (float)QuantumRange;
    float r = 0.5F * (ra + rb);
    float dR = ra - rb;
    float dG = ((float)a.greenQuantum() / (float)QuantumRange) - ((float)b.greenQuantum() / (float)QuantumRange);
    float dB = ((float)a.blueQuantum() / (float)QuantumRange) - ((float)b.blueQuantum() / (float)QuantumRange);
    return sqrt((2.0F + r) * dR * dR + 4.0F * dG * dG + (3.0F - r) * dB * dB);
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

std::vector<uint8_t> minimizeColorDistance(const std::vector<Color> &colors)
{
    // build map with color distance for all possible combinations from palette
    std::cout << "Building color distance map..." << std::endl;
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
    // insert colors / indices successively at optimal positions
    std::cout << "Finding optimal palette..." << std::endl;
    std::vector<uint8_t> currentIndices(1, 0);
    for (uint32_t i = 1; i < colors.size(); i++)
    {
        currentIndices = insertIndexOptimal(currentIndices, distancesSqrMap, i);
    }
    return currentIndices;
}

std::vector<Color> swapColors(const std::vector<Color> &colors, const std::vector<uint8_t> &newIndices)
{
    std::vector<Color> result;
    for (uint32_t i = 0; i < colors.size(); i++)
    {
        result.push_back(colors.at(newIndices[i]));
    }
    return result;
}
