#pragma once

#include "colorformat.h"
#include "exception.h"

#include <array>
#include <bit>
#include <cstdint>

namespace Color
{

    /// @brief Linear XRGB1555 color in range [0,31]
    class XRGB1555
    {
    public:
        using pixel_type = uint16_t; // pixel value type
        using value_type = uint8_t;  // color channel value type
        static constexpr Color::Format ColorFormat = Format::XRGB1555;
        static constexpr uint32_t Channels = 3;

        XRGB1555() : v(std::bit_cast<Value>(uint16_t(0))) {}

        XRGB1555(value_type R, value_type G, value_type B)
        {
            REQUIRE(R <= Max[0], std::runtime_error, "Red color out of range [0, 31]");
            REQUIRE(G <= Max[1], std::runtime_error, "Green color out of range [0, 31]");
            REQUIRE(B <= Max[2], std::runtime_error, "Blue color out of range [0, 31]");
            v.x = 0;
            v.r = R;
            v.g = G;
            v.b = B;
        }

        XRGB1555(const std::array<value_type, 3> &rgb)
        {
            REQUIRE(rgb[0] <= Max[0], std::runtime_error, "Red color out of range [0, 31]");
            REQUIRE(rgb[1] <= Max[1], std::runtime_error, "Green color out of range [0, 31]");
            REQUIRE(rgb[2] <= Max[2], std::runtime_error, "Blue color out of range [0, 31]");
            v.x = 0;
            v.r = rgb[0];
            v.g = rgb[1];
            v.b = rgb[2];
        }

        /// @brief Construct color using raw XRGB1555 value
        XRGB1555(uint16_t xrgb)
        {
            uint16_t temp = xrgb & uint16_t(0x7FFF);
            v = std::bit_cast<Value>(temp);
        }

        constexpr XRGB1555(const XRGB1555 &other) : v(other.v) {}

        inline XRGB1555 &operator=(const XRGB1555 &other)
        {
            v = other.v;
            return *this;
        }

        /// @brief Set color using raw XRGB1555 value
        inline XRGB1555 &operator=(uint16_t xrgb)
        {
            uint16_t temp = xrgb & uint16_t(0x7FFF);
            v = std::bit_cast<Value>(temp);
            return *this;
        }

        inline auto R() const -> value_type { return v.r; }
        inline auto G() const -> value_type { return v.g; }
        inline auto B() const -> value_type { return v.b; }

        inline auto operator[](std::size_t pos) const -> value_type { return pos == 0 ? v.r : (pos == 1 ? v.g : v.b); }

        /// @brief Return raw XRGB1555 value
        inline operator uint16_t() const { return std::bit_cast<uint16_t>(v); }

        static constexpr std::array<value_type, 3> Min{0, 0, 0};
        static constexpr std::array<value_type, 3> Max{31, 31, 31};

        /// @brief Return swapped red and blue color channel
        auto swappedRB() const -> XRGB1555;

        /// @brief Calculate square of perceived distance between colors
        /// See: https://stackoverflow.com/a/40950076 and https://www.compuphase.com/cmetric.htm
        /// @return Returns a value in [0,1]
        static auto distance(const XRGB1555 &color0, const XRGB1555 &color1) -> float;

    private:
        struct Value
        {
            uint16_t b : 5;
            uint16_t g : 5;
            uint16_t r : 5;
            uint16_t x : 1;
        } __attribute__((aligned(2), packed)) v; // low to high bits, BGRX in memory
    };

}
