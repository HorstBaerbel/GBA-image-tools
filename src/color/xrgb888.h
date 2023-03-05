#pragma once

#include <array>
#include <cstdint>
#include <string>

namespace Color
{

    /// @brief XRGB888 color in range [0,255]
    class XRGB888
    {
    public:
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

        inline auto x() const -> const uint8_t & { return v[0]; }
        inline auto x() -> uint8_t & { return v[0]; }
        inline auto y() const -> const uint8_t & { return v[1]; }
        inline auto y() -> uint8_t & { return v[1]; }
        inline auto z() const -> const uint8_t & { return v[2]; }
        inline auto z() -> uint8_t & { return v[2]; }

        inline operator uint32_t() const { return c; }

        static const XRGB888 Min;
        static const XRGB888 Max;

        /// @brief XRGB888 color from float RGB data
        static auto fromRGBf(float R, float G, float B) -> XRGB888;

        /// @brief XRGB888 color from double RGB data
        static auto fromRGBf(double R, double G, double B) -> XRGB888;

        /// @brief XRGB888 to hex color string
        auto toHex() const -> std::string;

        /// @brief XRGB888 to hex color string
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
