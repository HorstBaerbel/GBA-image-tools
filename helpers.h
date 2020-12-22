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
