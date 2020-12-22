// some data conversion utility functions used by the tools
#pragma once

#include <cstdint>
#include <string>
#include <cstring>
#include <vector>

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
