#pragma once

#include <algorithm>
#include <cstdint>
#include <map>
#include <numeric>
#include <vector>
#include <limits>

namespace Histogram
{

    template <typename T, typename F = uint64_t>
    auto buildHistogram(const std::vector<T> &data) -> std::map<T, F>
    {
        std::map<T, F> histogram;
        std::for_each(data.cbegin(), data.cend(), [&histogram](auto value)
                      { histogram[value]++; });
        return histogram;
    }

    template <typename T, typename F = uint64_t>
    auto buildHistogramKeepEmpty(const std::vector<T> &data) -> std::map<T, F>
    {
        std::map<T, F> histogram;
        for (std::size_t i = 0; i <= std::numeric_limits<T>::max(); ++i)
        {
            histogram[i] = 0;
        }
        std::for_each(data.cbegin(), data.cend(), [&histogram](auto value)
                      { histogram[value]++; });
        return histogram;
    }

    template <typename T, typename F = uint64_t>
    auto normalizeHistogram(const std::map<T, F> &histogram) -> std::map<T, float>
    {
        std::map<T, float> normalized;
        const F sum = std::accumulate(histogram.cbegin(), histogram.cend(), F(0), [](const auto &a, const auto &b)
                                      { return a + b.second; });
        std::transform(histogram.cbegin(), histogram.cend(), std::inserter(normalized, normalized.end()), [sum](const auto &entry)
                       { return std::make_pair(entry.first, static_cast<float>(static_cast<double>(entry.second) / sum)); });
        return normalized;
    }

}
