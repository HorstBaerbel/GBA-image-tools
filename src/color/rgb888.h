#pragma once

#include "colorformat.h"
#include "exception.h"

#include <array>
#include <bit>
#include <cstdint>
#include <string>

namespace Color
{

    /// @brief Linear RGB888 24-bit color in range [0,255]
    class RGB888
    {
    public:
        using pixel_type = std::array<uint8_t, 3>; // pixel value type
        using value_type = uint8_t;                // color channel value type
        static constexpr Color::Format ColorFormat = Format::RGB888;
        static constexpr uint32_t Channels = 3;

        RGB888() : v({0, 0, 0}){};

        constexpr RGB888(value_type R, value_type G, value_type B) : v({B, G, R}) {}

        constexpr RGB888(const std::array<value_type, 3> &rgb) : v({rgb[2], rgb[1], rgb[0]}) {}

        /// @brief Construct color using raw XRGB8888 value
        RGB888(uint32_t xrgb)
        {
            auto temp = std::bit_cast<std::array<value_type, 4>>(xrgb);
            v[0] = temp[0];
            v[1] = temp[1];
            v[2] = temp[2];
        }

        constexpr RGB888(const RGB888 &other) : v(other.v) {}

        inline RGB888 &operator=(const RGB888 &other)
        {
            v = other.v;
            return *this;
        }

        /// @brief Set color using raw XRGB8888 value
        inline RGB888 &operator=(uint32_t xrgb)
        {
            auto temp = std::bit_cast<std::array<value_type, 4>>(xrgb);
            v[0] = temp[0];
            v[1] = temp[1];
            v[2] = temp[2];
            return *this;
        }

        inline auto R() const -> const value_type & { return v[2]; }
        inline auto R() -> value_type & { return v[2]; }
        inline auto G() const -> const value_type & { return v[1]; }
        inline auto G() -> value_type & { return v[1]; }
        inline auto B() const -> const value_type & { return v[0]; }
        inline auto B() -> value_type & { return v[0]; }

        inline auto operator[](std::size_t pos) const -> const value_type & { return pos == 0 ? v[2] : (pos == 1 ? v[1] : v[0]); }
        inline auto operator[](std::size_t pos) -> value_type & { return pos == 0 ? v[2] : (pos == 1 ? v[1] : v[0]); }

        /// @brief Return raw XRGB8888 value
        inline operator uint32_t() const
        {
            std::array<uint8_t, 4> temp = {v[0], v[1], v[2], 0};
            return std::bit_cast<uint32_t>(temp);
        }

        static constexpr std::array<value_type, 3> Min{0, 0, 0};
        static constexpr std::array<value_type, 3> Max{255, 255, 255};

        /// @brief Return swapped red and blue color channel
        auto swappedRB() const -> RGB888;

        /// @brief Convert from 24-bit hex color string, with or w/o a prefix: RRGGBB or #RRGGBB
        static auto fromHex(const std::string &hex) -> RGB888;

        /// @brief Convert to 24-bit hex color string, excluding a prefix: RRGGBB
        auto toHex() const -> std::string;

        /// @brief Convert to 24-bit hex color string, excluding a prefix: RRGGBB
        static auto toHex(const RGB888 &color) -> std::string;

        /// @brief Calculate square of perceived distance between colors
        /// See: https://stackoverflow.com/a/40950076 and https://www.compuphase.com/cmetric.htm
        /// @return Returns a value in [0,1]
        static auto distance(const RGB888 &color0, const RGB888 &color1) -> float;

    private:
        std::array<uint8_t, 3> v; // BGR in memory
    };

}
