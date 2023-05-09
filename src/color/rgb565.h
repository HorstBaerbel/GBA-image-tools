#pragma once

#include "colorformat.h"

#include <array>
#include <cstdint>
#include <string>

namespace Color
{

    /// @brief Linear RGB565 color in range [0,31] resp. [0,63]
    class RGB565
    {
    public:
        static constexpr Color::Format ColorFormat = Format::RGB565;
        using pixel_type = uint16_t; // pixel value type
        using value_type = uint8_t;  // color channel value type

        RGB565() = default;
        constexpr RGB565(uint8_t R, uint8_t G, uint8_t B) : r(R), g(G), b(B) {}
        constexpr RGB565(uint16_t color) : c(color) {}

        inline RGB565 &operator=(const RGB565 &other)
        {
            c = other.c;
            return *this;
        }

        inline RGB565 &operator=(uint16_t color)
        {
            c = color;
            return *this;
        }

        inline auto R() const -> const uint8_t { return r; }
        inline auto G() const -> const uint8_t { return g; }
        inline auto B() const -> const uint8_t { return b; }

        inline auto raw() const -> pixel_type { return c; }

        inline operator uint16_t() const { return c; }

        static constexpr std::array<uint8_t, 3> Min{0, 0, 0};
        static constexpr std::array<uint8_t, 3> Max{31, 63, 31};

        /// @brief Return swapped red and blue color channel
        auto swappedRB() const -> RGB565;

        /// @brief Calculate square of perceived distance between colors
        /// See: https://stackoverflow.com/a/40950076 and https://www.compuphase.com/cmetric.htm
        /// @return Returns a value in [0,1]
        static auto distance(const RGB565 &color0, const RGB565 &color1) -> float;

    private:
#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wpedantic"
        union
        {
            struct
            {
                uint8_t r : 5;
                uint8_t g : 6;
                uint8_t b : 5;
            };              // RGB
            uint16_t c = 0; // BGR
        } __attribute__((aligned(2), packed));
#pragma GCC diagnostic pop
    };

}
