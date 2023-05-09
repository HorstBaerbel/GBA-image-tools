#pragma once

#include "colorformat.h"
#include "exception.h"

#include <array>
#include <bit>
#include <cstdint>
#include <string>

namespace Color
{

    /// @brief Linear XRGB1555 color in range [0,31]
    class XRGB1555
    {
    public:
        static constexpr Color::Format ColorFormat = Format::XRGB1555;
        using pixel_type = uint16_t; // pixel value type
        using value_type = uint8_t;  // color channel value type

        XRGB1555() = default;

        XRGB1555(uint8_t R, uint8_t G, uint8_t B)
        {
            REQUIRE(R <= Max[0], std::runtime_error, "Red color out of range [0, 31]");
            REQUIRE(G <= Max[1], std::runtime_error, "Green color out of range [0, 31]");
            REQUIRE(B <= Max[2], std::runtime_error, "Blue color out of range [0, 31]");
            v.r = R;
            v.g = G;
            v.b = B;
            v.x = 0;
        }

        /// @brief Construct color using raw XRGB1555 value
        XRGB1555(uint16_t xrgb)
        {
            REQUIRE((xrgb & 0x8000) == 0, std::runtime_error, "15th bit must be 0 for XRGB1555");
            v = std::bit_cast<Value>(xrgb);
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
            REQUIRE((xrgb & 0x8000) == 0, std::runtime_error, "15th bit must be 0 for XRGB1555");
            v = std::bit_cast<Value>(xrgb);
            return *this;
        }

        inline auto R() const -> const uint8_t { return v.r; }
        inline auto G() const -> const uint8_t { return v.g; }
        inline auto B() const -> const uint8_t { return v.b; }

        /// @brief Return raw XRGB1555 value
        inline auto raw() const -> pixel_type { return std::bit_cast<uint16_t>(v); }

        /// @brief Return raw XRGB1555 value
        inline operator uint16_t() const { return std::bit_cast<uint16_t>(v); }

        static constexpr std::array<uint8_t, 3> Min{0, 0, 0};
        static constexpr std::array<uint8_t, 3> Max{31, 31, 31};

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
        } __attribute__((aligned(2), packed)) v; // XRGB in memory
    };

}
