#pragma once

#include "colorformat.h"

#include <array>
#include <cstdint>
#include <string>

namespace Color
{

    /// @brief Linear XRGB8888 32-bit color in range [0,255]
    class XRGB888
    {
    public:
        static constexpr Color::Format ColorFormat = Format::XRGB888;
        using pixel_type = uint16_t; // pixel value type
        using value_type = uint8_t;  // color channel value type

        XRGB888() = default;
        constexpr XRGB888(uint8_t R, uint8_t G, uint8_t B) : v({R, G, B, 0}) {}
        constexpr XRGB888(uint32_t color) : c(color) {}

        inline XRGB888 &operator=(const XRGB888 &other)
        {
            c = other.c;
            return *this;
        }

        inline XRGB888 &operator=(uint32_t color)
        {
            c = color;
            return *this;
        }

        inline auto R() const -> const uint8_t & { return v[0]; }
        inline auto R() -> uint8_t & { return v[0]; }
        inline auto G() const -> const uint8_t & { return v[1]; }
        inline auto G() -> uint8_t & { return v[1]; }
        inline auto B() const -> const uint8_t & { return v[2]; }
        inline auto B() -> uint8_t & { return v[2]; }

        inline auto raw() const -> pixel_type { return c; }

        inline operator uint32_t() const { return c; }

        static constexpr std::array<uint8_t, 3> Min{0, 0, 0};
        static constexpr std::array<uint8_t, 3> Max{255, 255, 255};

        /// @brief Return swapped red and blue color channel
        auto swappedRB() const -> XRGB888;

        /// @brief Convert from 24-bit hex color string, with or w/o a prefix: RRGGBB or #RRGGBB
        static auto fromHex(const std::string &hex) -> XRGB888;

        /// @brief Convert to 24-bit hex color string, excluding a prefix: RRGGBB
        auto toHex() const -> std::string;

        /// @brief Convert to 24-bit hex color string, excluding a prefix: RRGGBB
        static auto toHex(const XRGB888 &color) -> std::string;

        /// @brief Calculate square of perceived distance between colors
        /// See: https://stackoverflow.com/a/40950076 and https://www.compuphase.com/cmetric.htm
        /// @return Returns a value in [0,1]
        static auto distance(const XRGB888 &color0, const XRGB888 &color1) -> float;

    private:
#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wpedantic"
        union
        {
            uint32_t c = 0;           // XBGR
            std::array<uint8_t, 4> v; // RGBX
        } __attribute__((aligned(4), packed));
#pragma GCC diagnostic pop
    };

}
