#pragma once

#include <cstdint>

namespace Color
{

    /// @brief XRGB888 color in range [0,255]
    class XRGB888
    {
    public:
        XRGB888() = default;
        constexpr XRGB888(uint8_t R, uint8_t G, uint8_t B) : r(R), g(G), b(B) {}
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

        inline auto R() const -> const uint8_t { return r; }
        inline auto R() -> uint8_t { return r; }
        inline auto G() const -> const uint8_t { return g; }
        inline auto G() -> uint8_t { return g; }
        inline auto B() const -> const uint8_t { return b; }
        inline auto B() -> uint8_t { return b; }

        inline operator uint32_t() const { return c; }

        /// @brief XRGB888 color from float RGB data
        static auto fromRGBf(float R, float G, float B) -> XRGB888;

        /// @brief XRGB888 color from double RGB data
        static auto fromRGBf(double R, double G, double B) -> XRGB888;

        /// @brief Calculate square of perceived distance between colors
        /// See: https://stackoverflow.com/a/40950076 and https://www.compuphase.com/cmetric.htm
        /// @return Returns a value in [0,1]
        static auto distance(const XRGB888 &color0, const XRGB888 &color1) -> float;

    private:
#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wpedantic"
        union
        {
            struct
            {
                uint8_t b;
                uint8_t g;
                uint8_t r;
                uint8_t x;
            } __attribute__((aligned(4), packed));
            uint32_t c = 0;
        } __attribute__((aligned(4), packed));
#pragma GCC diagnostic pop
    };

}
