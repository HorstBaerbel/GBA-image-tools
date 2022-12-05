// some data conversion utility functions used by the tools
#pragma once

#include <algorithm>
#include <cassert>
#include <cstdint>
#include <cstring>
#include <numeric>
#include <vector>

#include "exception.h"

/// @brief Fill up the vector with values until its size is a multiple of multipleOf.
template <typename T>
std::vector<T> fillUpToMultipleOf(const std::vector<T> &data, uint32_t multipleOf, T value = T())
{
    if (data.empty())
    {
        return {};
    }
    std::vector<T> result = data;
    const auto size = result.size();
    if (size < multipleOf)
    {
        result.resize(multipleOf, value);
    }
    else if (size % multipleOf != 0)
    {
        auto newSize = ((size + multipleOf - 1) / multipleOf) * multipleOf;
        result.resize(newSize, value);
    }
    REQUIRE((result.size() % multipleOf) == 0, std::runtime_error, "Size not filled up to a multiple of " << multipleOf << "!");
    return result;
}

/// @brief Concat all arrays and convert data from type T to type R by raw copy.
/// Use e.g. to convert multiple arrays of uint8_ts to a single uint32_t array.
template <typename R, typename T>
std::vector<R> combineTo(const std::vector<std::vector<T>> &data)
{
    size_t combinedSize = 0;
    for (const auto &current : data)
    {
        REQUIRE((current.size() * sizeof(T)) % sizeof(R) == 0, std::runtime_error, "Size must be a multiple of " << sizeof(R) << "!");
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
    REQUIRE((data.size() * sizeof(T)) % sizeof(R) == 0, std::runtime_error, "Size must be a multiple of " << sizeof(R) << "!");
    std::vector<R> result((data.size() * sizeof(T)) / sizeof(R));
    std::memcpy(result.data(), data.data(), data.size() * sizeof(T));
    return result;
}

/// @brief Return the start index of each sub-vector in the outer vector as if all vectors were concatenated.
template <typename T>
std::vector<uint32_t> getStartIndices(const std::vector<std::vector<T>> &data)
{
    size_t currentIndex = 0;
    std::vector<uint32_t> result;
    for (const auto &current : data)
    {
        result.push_back(currentIndex);
        currentIndex += current.size();
    }
    return result;
}

/// @brief Divide every element in the vector by a certain value.
template <typename T>
std::vector<T> divideBy(const std::vector<T> &data, T divideBy = 1)
{
    std::vector<T> result;
    std::transform(data.cbegin(), data.cend(), std::back_inserter(result), [divideBy](auto t)
                   { return t / divideBy; });
    return result;
}

/// @brief Interleave all pixel data: D0P0, D1P0, D0P1, D1P1...
std::vector<uint8_t> interleave(const std::vector<std::vector<uint8_t>> &data, uint32_t bitsPerPixel);

/// @brief Delta-encode data. First value is stored verbatim. All other values are stored as difference to previous value
template <typename T>
std::vector<T> deltaEncode(const std::vector<T> &data)
{
    std::vector<T> result;
    std::adjacent_difference(data.cbegin(), data.cend(), std::back_inserter(result));
    return result;
}

/// @brief Prepend value to array
template <typename T>
std::vector<uint8_t> prependValue(const std::vector<uint8_t> &data, T value)
{
    std::vector<uint8_t> result(data.size() + sizeof(T));
    *reinterpret_cast<T *>(result.data()) = value;
    std::copy(data.cbegin(), data.cend(), std::next(result.begin(), sizeof(T)));
    return result;
}
