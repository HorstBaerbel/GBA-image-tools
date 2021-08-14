#include "imagehelpers.h"

#include <stdexcept>

std::vector<uint8_t> getImageData(const Image &img)
{
    std::vector<uint8_t> data;
    if (img.type() == PaletteType)
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
    else if (img.type() == TrueColorType)
    {
        // get pixel colors and rescale to RGB555
        const auto nrOfPixels = img.columns() * img.rows();
        auto pixels = img.getConstPixels(0, 0, img.columns(), img.rows());
        for (std::remove_const<decltype(nrOfPixels)>::type i = 0; i < nrOfPixels; ++i)
        {
            auto pixel = pixels[i];
            data.push_back((31 * pixel.red) / QuantumRange);
            data.push_back((31 * pixel.green) / QuantumRange);
            data.push_back((31 * pixel.blue) / QuantumRange);
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

std::vector<uint8_t> convertDataToNibbles(const std::vector<uint8_t> &indices)
{
    std::vector<uint8_t> data;
    // size must be a multiple of 2
    const auto nrOfIndices = indices.size();
    if (nrOfIndices & 1)
    {
        throw std::runtime_error("Number of indices must be even!");
    }
    for (std::remove_const<decltype(nrOfIndices)>::type i = 0; i < nrOfIndices; i += 2)
    {
        if (indices[i] > 15 || indices[i + 1] > 15)
        {
            throw std::runtime_error("Index values must be < 16!");
        }
        uint8_t v = (((indices[i + 1]) & 0x0F) << 4);
        v |= ((indices[i]) & 0x0F);
        data.push_back(v);
    }
    return data;
}

std::vector<uint8_t> incImageIndicesBy1(const std::vector<uint8_t> &imageData)
{
    auto tempData = imageData;
    std::for_each(tempData.begin(), tempData.end(), [](auto &index)
                  { index++; });
    return tempData;
}

std::vector<uint8_t> swapIndexToIndex0(std::vector<uint8_t> &imageData, uint8_t oldIndex)
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

std::vector<uint8_t> swapIndices(std::vector<uint8_t> &imageData, const std::vector<uint8_t> &newIndices)
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
