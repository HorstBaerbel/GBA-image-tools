#include "imagehelpers.h"

#include "colorhelpers.h"
#include "datahelpers.h"
#include "exception.h"

std::vector<uint8_t> getImageData(const Magick::Image &img)
{
    std::vector<uint8_t> data;
    if (img.type() == Magick::ImageType::PaletteType)
    {
        const auto nrOfColors = img.colorMapSize();
        const auto nrOfIndices = img.columns() * img.rows();
        auto pixels = img.getConstPixels(0, 0, img.columns(), img.rows()); // we need to call this first for getIndices to work...
        auto indices = img.getConstIndexes();
        if (nrOfColors <= 256)
        {
            for (std::remove_const<decltype(nrOfIndices)>::type i = 0; i < nrOfIndices; ++i)
            {
                data.push_back(indices[i]);
            }
        }
        else
        {
            throw std::runtime_error("Only up to 256 colors supported in color map!");
        }
    }
    else if (img.type() == Magick::ImageType::TrueColorType)
    {
        // get pixel colors aas RGB888
        const auto nrOfPixels = img.columns() * img.rows();
        auto pixels = img.getConstPixels(0, 0, img.columns(), img.rows());
        for (std::remove_const<decltype(nrOfPixels)>::type i = 0; i < nrOfPixels; ++i)
        {
            auto pixel = pixels[i];
            data.push_back(static_cast<uint8_t>(std::round(255.0 * Magick::Color::scaleQuantumToDouble(pixel.red))));
            data.push_back(static_cast<uint8_t>(std::round(255.0 * Magick::Color::scaleQuantumToDouble(pixel.green))));
            data.push_back(static_cast<uint8_t>(std::round(255.0 * Magick::Color::scaleQuantumToDouble(pixel.blue))));
        }
    }
    else
    {
        throw std::runtime_error("Unsupported image type!");
    }
    /*std::ofstream of("dump.hex", std::ios::binary | std::ios::out);
    of.write(reinterpret_cast<const char *>(data.data()), data.size());
    of.close();*/
    return data;
}

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

std::vector<uint8_t> convertDataTo1Bit(const std::vector<uint8_t> &indices)
{
    std::vector<uint8_t> result;
    // size must be a multiple of 8
    const auto nrOfIndices = indices.size();
    REQUIRE((nrOfIndices & 7) == 0, std::runtime_error, "Number of indices must be divisible by 8");
    for (std::remove_const<decltype(nrOfIndices)>::type i = 0; i < nrOfIndices; i += 8)
    {
        REQUIRE(indices[i] <= 1 || indices[i + 1] <= 1 || indices[i + 2] <= 1 || indices[i + 3] <= 1 || indices[i + 4] <= 1 || indices[i + 5] <= 1 || indices[i + 6] <= 1 || indices[i + 7] <= 1, std::runtime_error, "Index values must be < 4");
        uint8_t v = (((indices[i + 7]) & 0x01) << 7);
        v |= (((indices[i + 6]) & 0x01) << 6);
        v |= (((indices[i + 5]) & 0x01) << 5);
        v |= (((indices[i + 4]) & 0x01) << 4);
        v |= (((indices[i + 3]) & 0x01) << 3);
        v |= (((indices[i + 2]) & 0x01) << 2);
        v |= (((indices[i + 1]) & 0x01) << 1);
        v |= ((indices[i]) & 0x01);
        result.push_back(v);
    }
    return result;
}

std::vector<uint8_t> convertDataTo2Bit(const std::vector<uint8_t> &indices)
{
    std::vector<uint8_t> result;
    // size must be a multiple of 4
    const auto nrOfIndices = indices.size();
    REQUIRE((nrOfIndices & 3) == 0, std::runtime_error, "Number of indices must be divisible by 4");
    for (std::remove_const<decltype(nrOfIndices)>::type i = 0; i < nrOfIndices; i += 4)
    {
        REQUIRE(indices[i] <= 3 || indices[i + 1] <= 3 || indices[i + 2] <= 3 || indices[i + 3] <= 3, std::runtime_error, "Index values must be < 4");
        uint8_t v = (((indices[i + 3]) & 0x03) << 6);
        v |= (((indices[i + 2]) & 0x03) << 4);
        v |= (((indices[i + 1]) & 0x03) << 2);
        v |= ((indices[i]) & 0x03);
        result.push_back(v);
    }
    return result;
}

std::vector<uint8_t> convertDataTo4Bit(const std::vector<uint8_t> &indices)
{
    std::vector<uint8_t> result;
    // size must be a multiple of 2
    const auto nrOfIndices = indices.size();
    REQUIRE((nrOfIndices & 1) == 0, std::runtime_error, "Number of indices must be even");
    for (std::remove_const<decltype(nrOfIndices)>::type i = 0; i < nrOfIndices; i += 2)
    {
        REQUIRE(indices[i] <= 15 || indices[i + 1] <= 15, std::runtime_error, "Index values must be < 16");
        uint8_t v = (((indices[i + 1]) & 0x0F) << 4);
        v |= ((indices[i]) & 0x0F);
        result.push_back(v);
    }
    return result;
}

std::vector<uint8_t> incImageIndicesBy1(const std::vector<uint8_t> &imageData)
{
    auto tempData = imageData;
    std::for_each(tempData.begin(), tempData.end(), [](auto &index)
                  { index++; });
    return tempData;
}

std::vector<uint8_t> swapIndexToIndex0(const std::vector<uint8_t> &imageData, uint8_t oldIndex)
{
    auto tempData = imageData;
    for (size_t i = 0; i < tempData.size(); ++i)
    {
        if (tempData[i] == oldIndex)
        {
            tempData[i] = 0;
        }
        else if (tempData[i] == 0)
        {
            tempData[i] = oldIndex;
        }
    }
    return tempData;
}

std::vector<uint8_t> swapIndices(const std::vector<uint8_t> &imageData, const std::vector<uint8_t> &newIndices)
{
    std::vector<uint8_t> reverseIndices(newIndices.size(), 0);
    for (uint32_t i = 0; i < newIndices.size(); i++)
    {
        reverseIndices[newIndices[i]] = i;
    }
    auto tempData = imageData;
    std::for_each(tempData.begin(), tempData.end(), [&reverseIndices](auto &i)
                  { i = reverseIndices[i]; });
    return tempData;
}

uint32_t getMaxNrOfColors(Magick::ImageType imgType, const std::vector<std::vector<Magick::Color>> &colorMaps)
{
    REQUIRE(imgType == Magick::ImageType::PaletteType, std::runtime_error, "Paletted images required");
    REQUIRE(!colorMaps.empty(), std::runtime_error, "No color maps passed");
    const auto &refColorMap = colorMaps.front();
    uint32_t maxColorMapColors = refColorMap.size();
    for (const auto &cm : colorMaps)
    {
        if (cm.size() > maxColorMapColors)
        {
            maxColorMapColors = cm.size();
        }
    }
    return maxColorMapColors;
}

std::vector<Magick::Color> padColorMap(const std::vector<Magick::Color> &colorMap, uint32_t nrOfColors)
{
    REQUIRE(!colorMap.empty(), std::runtime_error, "Empty color map passed");
    REQUIRE(nrOfColors <= 256, std::runtime_error, "Can't padd color map to more than 256 colors");
    // padd all color maps to max color count
    return fillUpToMultipleOf(colorMap, nrOfColors);
}
