#pragma once

#include "exception.h"

#include <cstdint>
#include <limits>
#include <array>
#include <type_traits>

namespace Image
{

    /// @brief Value-type dimension of the stored image data
    class DataSize
    {
    public:
        constexpr DataSize() = default;
        constexpr DataSize(uint32_t width, uint32_t height) : m_size{width, height} {}
        template <typename T, typename std::enable_if<std::is_same<T, std::size_t>::value>::type...>
        DataSize(T width, T height) : m_size{static_cast<uint32_t>(width), static_cast<uint32_t>(height)}
        {
            REQUIRE(width <= std::numeric_limits<uint32_t>::max(), std::runtime_error, "Bad width value");
            REQUIRE(height <= std::numeric_limits<uint32_t>::max(), std::runtime_error, "Bad height value");
        }

        auto width() const -> uint32_t
        {
            return m_size[0];
        }

        auto height() const -> uint32_t
        {
            return m_size[1];
        }

        friend bool operator==(const DataSize &lhs, const DataSize &rhs);
        friend bool operator!=(const DataSize &lhs, const DataSize &rhs);

    private:
        std::array<uint32_t, 2> m_size = {0, 0};
    };

    inline bool operator==(const DataSize &lhs, const DataSize &rhs)
    {
        return lhs.m_size == rhs.m_size;
    }

    inline bool operator!=(const DataSize &lhs, const DataSize &rhs)
    {
        return !(lhs == rhs);
    }

}