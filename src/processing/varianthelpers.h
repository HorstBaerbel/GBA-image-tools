#pragma once

#include <tuple>
#include <variant>
#include <vector>

namespace VariantHelpers
{

    template <std::size_t... Is, class... Args, class T>
    auto hasTypes(const std::vector<T> &v, const std::tuple<Args...> &types, std::index_sequence<Is...>) -> bool
    {
        if (sizeof...(Args) != v.size())
        {
            return false;
        }
        std::size_t correctTypes = 0;
        (void(correctTypes += (std::holds_alternative<Args>(v[Is])) ? 1 : 0), ...);
        return correctTypes == sizeof...(Args);
    }

    /// @brief Check if vector of variants contains correct types
    /// @tparam T Vector content type == variant type
    /// @tparam Args List of types the vector must contain in this order
    /// @param v Vector of variants to check
    template <class... Args, class T>
    auto hasTypes(const std::vector<T> &v) -> bool
    {
        return HasTypes(v, std::tuple<Args...>(), std::make_index_sequence<sizeof...(Args)>{});
    }

    /// @brief Check if vector of variants contains correct types
    /// @tparam T Vector content type == variant type
    /// @tparam V Value type we want to get
    /// @tparam index Index of value we want to get
    /// @param v Vector of variants get value from
    template <class V, std::size_t index, class T>
    auto getValue(const std::vector<T> &v) -> V
    {
        return std::get<V>(v[index]);
    }

}