#pragma once

#include <cstdint>
#include <map>
#include <numeric>
#include <vector>

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
    auto normalizeHistogram(const std::map<T, uint64_t> &histogram) -> std::map<T, float>
    {
        std::map<T, float> normalized;
        const uint64_t sum = std::accumulate(histogram.cbegin(), histogram.cend(), uint64_t(0));
        std::transform(histogram.cbegin(), histogram.cend(), std::inserter(normalized, normalized.end()), [sum](const auto &entry)
                       { return {entry.first, static_cast<float>(static_cast<double>(entry.second) / sum)}; });
        return normalized;
    }

}
