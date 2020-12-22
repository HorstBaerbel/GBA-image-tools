#include "colorhelpers.h"

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

std::vector<Color> addColorAtIndex0(const std::vector<Color> &colorMap, const Color &color0)
{
    std::vector<Color> tempMap(colorMap.size() + 1, color0);
    std::copy(colorMap.cbegin(), colorMap.cend(), std::next(tempMap.begin()));
    return tempMap;
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
