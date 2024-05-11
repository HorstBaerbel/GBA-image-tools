#pragma once

#include "colorformat.h"
#include "exception.h"

#include <array>
#include <bit>
#include <cstdint>
#include <string>

namespace Color
{

    /// @brief Linear XRGB8888 32-bit color in range [0,255]
    class XRGB8888
    {
    public:
        using pixel_type = uint32_t; // pixel value type
        using value_type = uint8_t;  // color channel value type
        static constexpr Color::Format ColorFormat = Format::XRGB8888;
        static constexpr uint32_t Channels = 3;

        XRGB8888() : v({0, 0, 0, 0}){};

        constexpr XRGB8888(value_type R, value_type G, value_type B) : v({B, G, R, 0}) {}

        constexpr XRGB8888(const std::array<value_type, 3> &rgb) : v({rgb[2], rgb[1], rgb[0], 0}) {}

        /// @brief Construct color using raw XRGB8888 value
        XRGB8888(uint32_t xrgb)
        {
            v = std::bit_cast<std::array<value_type, 4>>(xrgb);
            v[3] = 0;
        }

        constexpr XRGB8888(const XRGB8888 &other) : v(other.v) {}

        inline XRGB8888 &operator=(const XRGB8888 &other)
        {
            v = other.v;
            return *this;
        }

        /// @brief Set color using raw XRGB8888 value
        inline XRGB8888 &operator=(uint32_t xrgb)
        {
            v = std::bit_cast<std::array<value_type, 4>>(xrgb);
            v[3] = 0;
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
        explicit inline operator uint32_t() const { return std::bit_cast<uint32_t>(v); }

        static constexpr std::array<value_type, 3> Min{0, 0, 0};
        static constexpr std::array<value_type, 3> Max{255, 255, 255};

        /// @brief Return swapped red and blue color channel
        auto swappedRB() const -> XRGB8888;

        /// @brief Convert from 24-bit hex color string, with or w/o a prefix: RRGGBB or #RRGGBB
        static auto fromHex(const std::string &hex) -> XRGB8888;

        /// @brief Convert to 24-bit hex color string, excluding a prefix: RRGGBB
        auto toHex() const -> std::string;

        /// @brief Convert to 24-bit hex color string, excluding a prefix: RRGGBB
        static auto toHex(const XRGB8888 &color) -> std::string;

        /// @brief Calculate mean squared error between colors using simple metric
        /// @return Returns a value in [0,1]
        /// See: https://stackoverflow.com/a/40950076 and https://www.compuphase.com/cmetric.htm
        static auto mse(const XRGB8888 &color0, const XRGB8888 &color1) -> float;

        friend bool operator==(const XRGB8888 &c1, const XRGB8888 &c2);
        friend bool operator!=(const XRGB8888 &c1, const XRGB8888 &c2);

    private:
        std::array<uint8_t, 4> v; // BGRX in memory
    };

}
