#pragma once

#include <cstdint>
#include <map>
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

}
