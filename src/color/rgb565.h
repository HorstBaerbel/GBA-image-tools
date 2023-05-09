#pragma once

#include "colorformat.h"
#include "exception.h"

#include <array>
#include <bit>
#include <cstdint>

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

        RGB565(uint8_t R, uint8_t G, uint8_t B)
        {
            REQUIRE(R <= Max[0], std::runtime_error, "Red color out of range [0, 31]");
            REQUIRE(G <= Max[1], std::runtime_error, "Green color out of range [0, 63]");
            REQUIRE(B <= Max[2], std::runtime_error, "Blue color out of range [0, 31]");
            v.r = R;
            v.g = G;
            v.b = B;
        }

        /// @brief Construct color using raw RGB565 value
        constexpr RGB565(uint16_t rgb) : v(std::bit_cast<Value>(rgb)) {}

        inline RGB565 &operator=(const RGB565 &other)
        {
            v = other.v;
            return *this;
        }

        /// @brief Set color using raw RGB565 value
        inline RGB565 &operator=(uint16_t rgb)
        {
            v = std::bit_cast<Value>(rgb);
            return *this;
        }

        inline auto R() const -> const uint8_t { return v.r; }
        inline auto G() const -> const uint8_t { return v.g; }
        inline auto B() const -> const uint8_t { return v.b; }

        /// @brief Return raw RGB565 value
        inline auto raw() const -> pixel_type { return std::bit_cast<uint16_t>(v); }

        /// @brief Return raw RGB565 value
        inline operator uint16_t() const { return std::bit_cast<uint16_t>(v); }

        static constexpr std::array<uint8_t, 3> Min{0, 0, 0};
        static constexpr std::array<uint8_t, 3> Max{31, 63, 31};

        /// @brief Return swapped red and blue color channel
        auto swappedRB() const -> RGB565;

        /// @brief Calculate square of perceived distance between colors
        /// See: https://stackoverflow.com/a/40950076 and https://www.compuphase.com/cmetric.htm
        /// @return Returns a value in [0,1]
        static auto distance(const RGB565 &color0, const RGB565 &color1) -> float;

    private:
        struct Value
        {
            uint16_t b : 5;
            uint16_t g : 6;
            uint16_t r : 5;
        } __attribute__((aligned(2), packed)) v; // RGB in memory
    };

}
