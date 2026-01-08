#pragma once

#include "colorformat.h"
#include "exception.h"

#include <array>
#include <bit>
#include <cstdint>

namespace Color
{

    /// @brief sRGB RGB565 color in range [0,31] resp. [0,63]
    class RGB565
    {
    public:
        using pixel_type = uint16_t; // pixel value type
        using value_type = uint8_t;  // color channel value type
        static constexpr Color::Format ColorFormat = Format::RGB565;
        static constexpr uint32_t Channels = 3;

        RGB565() : v(std::bit_cast<Value>(uint16_t(0))) {}

        RGB565(value_type R, value_type G, value_type B)
        {
            REQUIRE(R <= Max[0], std::runtime_error, "Red color out of range [0, 31]");
            REQUIRE(G <= Max[1], std::runtime_error, "Green color out of range [0, 63]");
            REQUIRE(B <= Max[2], std::runtime_error, "Blue color out of range [0, 31]");
            v.r = R;
            v.g = G;
            v.b = B;
        }

        RGB565(const std::array<value_type, 3> &rgb)
        {
            REQUIRE(rgb[0] <= Max[0], std::runtime_error, "Red color out of range [0, 31]");
            REQUIRE(rgb[1] <= Max[1], std::runtime_error, "Green color out of range [0, 63]");
            REQUIRE(rgb[2] <= Max[2], std::runtime_error, "Blue color out of range [0, 31]");
            v.r = rgb[0];
            v.g = rgb[1];
            v.b = rgb[2];
        }

        /// @brief Construct color using raw RGB565 value
        constexpr RGB565(uint16_t rgb) : v(std::bit_cast<Value>(rgb)) {}

        constexpr RGB565(const RGB565 &other) : v(other.v) {}

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

        inline auto R() const -> value_type { return v.r; }
        inline auto G() const -> value_type { return v.g; }
        inline auto B() const -> value_type { return v.b; }

        inline auto operator[](std::size_t pos) const -> value_type { return pos == 0 ? v.r : (pos == 1 ? v.g : v.b); }

        /// @brief Return raw RGB565 value
        explicit inline operator uint16_t() const { return std::bit_cast<uint16_t>(v); }

        static constexpr std::array<value_type, 3> Min{0, 0, 0};
        static constexpr std::array<value_type, 3> Max{31, 63, 31};

        /// @brief Return swapped red and blue color channel
        auto swapToBGR() const -> RGB565;

        /// @brief Calculate mean squared error between colors using simple metric
        /// @return Returns a value in [0,1]
        /// See: https://stackoverflow.com/a/40950076 and https://www.compuphase.com/cmetric.htm
        static auto mse(const RGB565 &color0, const RGB565 &color1) -> float;

        friend bool operator==(const RGB565 &c1, const RGB565 &c2);
        friend bool operator!=(const RGB565 &c1, const RGB565 &c2);

    private:
        struct Value
        {
            uint16_t b : 5;
            uint16_t g : 6;
            uint16_t r : 5;
        } __attribute__((aligned(2), packed)) v; // low to high bits, BGR in memory
    };

}

// Specialization of std::hash for using class in std::map
template <>
struct std::hash<Color::RGB565>
{
    std::size_t operator()(const Color::RGB565 &c) const noexcept
    {
        return static_cast<Color::RGB565::pixel_type>(c);
    }
};

// Specialization of std::less for using class in std::map
template <>
struct std::less<Color::RGB565>
{
    bool operator()(const Color::RGB565 &lhs, const Color::RGB565 &rhs) const noexcept
    {
        return static_cast<Color::RGB565::pixel_type>(lhs) < static_cast<Color::RGB565::pixel_type>(rhs);
    }
};