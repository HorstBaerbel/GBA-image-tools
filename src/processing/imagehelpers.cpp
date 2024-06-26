#include "imagehelpers.h"

#include "color/colorhelpers.h"
#include "datahelpers.h"
#include "exception.h"

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

std::string imageTypeToString(const Magick::ImageType type)
{
    std::array<std::string, 11> imageTypes = {"BilevelType", "GrayscaleType", "GrayscaleAlphaType", "PaletteType", "PaletteAlphaType", "TrueColorType",
                                              "TrueColorAlphaType", "ColorSeparationType", "ColorSeparationAlphaType", "OptimizeType", "PaletteBilevelAlphaType"};
    auto index = ((uint32_t)type);
    if (index > 0 && index <= imageTypes.size())
    {
        return imageTypes[index - 1];
    }
    return "unknown";
}

std::string classTypeToString(const Magick::ClassType type)
{
    std::string classTypeString;
    std::array<std::string, 2> classTypes = {"DirectClass", "PseudoClass"};
    auto index = ((uint32_t)type);
    if (index > 0 && index <= classTypes.size())
    {
        return classTypes[index - 1];
    }
    return "unknown";
}

std::vector<uint8_t> getImageData(const Magick::Image &img)
{
    std::vector<uint8_t> data;
    if (img.classType() == Magick::ClassType::PseudoClass && img.type() == Magick::ImageType::GrayscaleType)
    {
        // get GrayChannel from image as unsigned chars
        auto temp = img.separate(MagickCore::GrayChannel);
        auto indices = temp.getConstPixels(0, 0, temp.columns(), temp.rows());
        REQUIRE(indices != nullptr, std::runtime_error, "Failed to get grayscale image pixels");
        const auto nrOfIndices = temp.columns() * temp.rows();
        for (std::remove_const<decltype(nrOfIndices)>::type i = 0; i < nrOfIndices; i++)
        {
            data.push_back(static_cast<uint8_t>(std::round(255.0F * indices[i]) / QuantumRange));
        }
    }
    else if (img.classType() == Magick::ClassType::PseudoClass && img.type() == Magick::ImageType::PaletteType)
    {
        // get palette indices as unsigned chars
        const auto nrOfColors = img.colorMapSize();
        REQUIRE(nrOfColors <= 256, std::runtime_error, "Only up to 256 colors supported in color map");
        // get IndexChannel from image
        auto temp = img.separate(MagickCore::IndexChannel);
        auto indices = temp.getConstPixels(0, 0, temp.columns(), temp.rows());
        REQUIRE(indices != nullptr, std::runtime_error, "Failed to get paletted image pixels");
        const auto nrOfIndices = temp.columns() * temp.rows();
        for (std::remove_const<decltype(nrOfIndices)>::type i = 0; i < nrOfIndices; i++)
        {
            REQUIRE(indices[i] <= 255, std::runtime_error, "Image color index must be <= 255");
            data.push_back(static_cast<uint8_t>(indices[i]));
        }
    }
    else if (img.classType() == Magick::ClassType::DirectClass && (img.type() == Magick::ImageType::TrueColorType || img.type() == Magick::ImageType::TrueColorAlphaType))
    {
        // get pixel colors as RGB888
        auto pixels = img.getConstPixels(0, 0, img.columns(), img.rows());
        REQUIRE(pixels != nullptr, std::runtime_error, "Failed to get truecolor image pixels");
        const auto nrOfPixels = img.columns() * img.rows();
        for (std::remove_const<decltype(nrOfPixels)>::type i = 0; i < nrOfPixels; i++)
        {
            data.push_back(static_cast<uint8_t>(std::round(255.0F * *pixels++) / QuantumRange));
            data.push_back(static_cast<uint8_t>(std::round(255.0F * *pixels++) / QuantumRange));
            data.push_back(static_cast<uint8_t>(std::round(255.0F * *pixels++) / QuantumRange));
            // ignore alpha channel pixels
            if (img.type() == Magick::ImageType::TrueColorAlphaType)
            {
                pixels++;
            }
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
    std::vector<Magick::Color> colorMap;
    if (img.classType() == Magick::ClassType::PseudoClass && img.type() == Magick::ImageType::GrayscaleType)
    {
        for (std::vector<Magick::Color>::size_type i = 0; i < 256; ++i)
        {
            double grayValue = i / 255.0;
            colorMap.push_back(Magick::ColorRGB(grayValue, grayValue, grayValue));
        }
    }
    else if (img.classType() == Magick::ClassType::PseudoClass && img.type() == Magick::ImageType::PaletteType)
    {
        const auto nrOfColors = img.colorMapSize();
        REQUIRE(nrOfColors <= 256, std::runtime_error, "Only up to 256 colors supported in color map");
        for (std::remove_const<decltype(nrOfColors)>::type i = 0; i < nrOfColors; ++i)
        {
            colorMap.push_back(img.colorMap(i));
        }
    }
    else
    {
        throw std::runtime_error("Unsupported image type!");
    }
    return colorMap;
}

void setColorMap(Magick::Image &img, const std::vector<Magick::Color> &colorMap)
{
    const auto nrOfColors = colorMap.size();
    REQUIRE(nrOfColors <= 256, std::runtime_error, "Only up to 256 colors supported in color map");
    for (std::remove_const<decltype(nrOfColors)>::type i = 0; i < nrOfColors; ++i)
    {
        img.colorMap(i, colorMap.at(i));
    }
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
