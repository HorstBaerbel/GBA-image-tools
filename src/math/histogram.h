#pragma once

#include <algorithm>
#include <cstdint>
#include <map>
#include <numeric>
#include <vector>
#include <limits>

namespace Histogram
{

    template <typename T>
    auto buildHistogram(const std::vector<T> &data) -> std::map<T, uint64_t>
    {
        std::map<T, uint64_t> histogram;
        std::for_each(data.cbegin(), data.cend(), [&histogram](auto value)
                      { histogram[value]++; });
        return histogram;
    }

    template <typename T>
    auto buildHistogramKeepEmpty(const std::vector<T> &data) -> std::map<T, uint64_t>
    {
        std::map<T, uint64_t> histogram;
        for (std::size_t i = 0; i <= std::numeric_limits<T>::max(); ++i)
        {
            histogram[i] = 0;
        }
        std::for_each(data.cbegin(), data.cend(), [&histogram](auto value)
                      { histogram[value]++; });
        return histogram;
    }

    template <typename T>
    auto normalizeHistogram(const std::map<T, uint64_t> &histogram) -> std::map<T, float>
    {
        std::map<T, float> normalized;
        const uint64_t sum = std::accumulate(histogram.cbegin(), histogram.cend(), uint64_t(0), [](const auto &a, const auto &b)
                                             { return a + b.second; });
        std::transform(histogram.cbegin(), histogram.cend(), std::inserter(normalized, normalized.end()), [sum](const auto &entry)
                       { return std::make_pair(entry.first, static_cast<float>(static_cast<double>(entry.second) / sum)); });
        return normalized;
    }

}
