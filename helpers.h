// some utility functions used by the tools
#pragma once

#include <cstdint>
#include <memory>
#include <string>
#include <cstring>
#include <vector>
#include <iomanip>
#include <iostream>
#include <fstream>
#include <Magick++.h>

using namespace Magick;

/// @brief Sprintf for std::string.
template <typename... Args>
std::string stringSprintf(const std::string &format, Args... args)
{
    size_t size = snprintf(nullptr, 0, format.c_str(), args...) + 1; // Extra space for '\0'
    std::unique_ptr<char[]> buf(new char[size]);
    snprintf(buf.get(), size, format.c_str(), args...);
    return std::string(buf.get(), buf.get() + size - 1); // We don't want the '\0' inside
}

/// @brief Fill up the vector with values until its size is a multiple of multipleOf.
template <typename T>
std::vector<T> &fillUpToMultipleOf(std::vector<T> &data, uint32_t multipleOf, T value = T())
{
    const auto size = data.size();
    if (size < multipleOf)
    {
        data.resize(multipleOf, value);
    }
    else if (size % multipleOf != 0)
    {
        data.resize(size + (size % multipleOf), value);
    }
    return data;
}

/// @brief Concat all arrays and convert data from type T to type R by raw copy.
/// Use e.g. to convert multiple arrays of uint8_ts to a single uint32_t array.
template <typename R, typename T>
std::vector<R> combineTo(const std::vector<std::vector<T>> &data)
{
    size_t combinedSize = 0;
    for (const auto &current : data)
    {
        if ((current.size() * sizeof(T)) % sizeof(R) != 0)
        {
            std::string message = "Size must be a multiple of " + std::to_string(sizeof(R)) + "!";
            throw std::runtime_error(message);
        }
        combinedSize += (current.size() * sizeof(T)) / sizeof(R);
    }
    std::vector<R> result(combinedSize);
    R *dst = result.data();
    for (const auto &current : data)
    {
        std::memcpy(dst, current.data(), current.size() * sizeof(T));
        dst += (current.size() * sizeof(T)) / sizeof(R);
    }
    return result;
}

/// @brief Convert data from type T to type R by raw copy.
/// Use e.g. to convert multiple arrays of uint8_ts to a single uint32_t array.
template <typename R, typename T>
std::vector<R> convertTo(const std::vector<T> &data)
{
    if ((data.size() * sizeof(T)) % sizeof(R) != 0)
    {
        std::string message = "Size must be a multiple of " + std::to_string(sizeof(R)) + "!";
        throw std::runtime_error(message);
    }
    std::vector<R> result((data.size() * sizeof(T)) / sizeof(R));
    std::memcpy(result.data(), data.data(), data.size() * sizeof(T));
    return result;
}

/// @brief Write values as a comma-separated array of hex numbers.
template <typename T>
void writeValues(std::ofstream &outFile, const std::vector<T> &data, bool asHex = false)
{
    auto flags = outFile.flags();
    size_t loopCount = 0;
    for (auto current : data)
    {
        if (asHex)
        {
            outFile << "0x" << std::hex << std::noshowbase << std::setw(sizeof(T) * 2) << std::setfill('0') << current;
        }
        else
        {
            outFile << std::dec << current;
        }
        if (loopCount < data.size() - 1)
        {
            outFile << ", ";
        }
        if (++loopCount % 10 == 0)
        {
            outFile << std::endl;
        }
    }
    outFile.flags(flags);
}

/// @brief Get ImageMagick image data (palette or truecolor) as raw data bytes.
/// If the palette has < 16 entries, two (nibble-sized) indices will be combined into a byte.
std::vector<uint8_t> getImageData(const Image & img);

/// @brief Get color map from ImageMagick Image.
std::vector<Color> getColorMap(const Image &img);
/// @brief set color map in an ImageMagick Image.
void setColorMap(Image &img, const std::vector<Color> &colorMap);

/// @brief Cut data to tileWidth * height pixel wide tiles. Width and height and tileWidth MUST be a multiple of 8!
std::vector<uint8_t> convertToWidth(const std::vector<uint8_t> &data, uint32_t width, uint32_t height, uint32_t bytesPerTile, uint32_t tileWidth);

/// @brief Cut data to 8x8 pixel wide tiles and store per tile instead of per scanline.
/// Width and height MUST be a multiple of 8!
std::vector<uint8_t> convertToTiles(const std::vector<uint8_t> &data, uint32_t width, uint32_t height, uint32_t bytesPerTile, uint32_t tileWidth = 8, uint32_t tileHeight = 8);

/// @brief Cut data to 8x8 pixel wide tiles and store per sprite instead of per scanline.
/// Width and height MUST be a multiple of 8 and of spriteWidth and spriteHeight.
std::vector<uint8_t> convertToSprites(const std::vector<uint8_t> &src, uint32_t width, uint32_t height, uint32_t bitsPerPixel, uint32_t spriteWidth, uint32_t spriteHeight);

/// @brief Convert ImageMagick Colors to RGB555 colors for GBA.
std::vector<uint16_t> convertToBGR555(const std::vector<Color> &colors);

/// @brief Convert a ImageMagick Color to a RGB555 color for GBA.
uint16_t colorToBGR555(const Color &color);

/// @brief Write image information to a .h file. 
void writeImageInfoToH(std::ofstream &hFile, const std::string &varName, const std::vector<uint32_t> &data, uint32_t width, uint32_t height, uint32_t bytesPerImage, uint32_t nrOfImages = 1, bool asTiles = false);
/// @brief Write additional palette information to a .h file. Use after write writeImageInfoToH.
void writePaletteInfoToHeader(std::ofstream &hFile, const std::string &varName, const std::vector<uint16_t> &data, uint32_t nrOfColors, bool singleColorMap = true, bool asTiles = false);
/// @brief Write image data to a .c file. 
void writeImageDataToC(std::ofstream &cFile, const std::string &varName, const std::string &hFileBaseName, const std::vector<uint32_t> &data, const std::vector<uint32_t> &startIndices = std::vector<uint32_t>(), bool asTiles = false);
/// @brief Write palette data to a .c file. Use after write writeImageDataToC.
void writePaletteDataToC(std::ofstream &cFile, const std::string &varName, const std::vector<uint16_t> &data, const std::vector<uint32_t> &startIndices = std::vector<uint32_t>(), bool asTiles = false);

/// @brief Get base name from filepath.
std::string getBaseNameFromFilePath(const std::string &filePath);

/// @brief Try to convert string to integer and return <success, value>.
std::pair<bool, uint32_t> stringToUint(const std::string &string);
