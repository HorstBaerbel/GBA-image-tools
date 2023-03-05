#pragma once

#include <Eigen/Core>
#include <cstdint>

namespace Color
{

    /// @brief Floating point RGB color in range [0,1]
    class RGBf : public Eigen::Vector3f
    {
    public:
        RGBf() = default;
        RGBf(const Eigen::Vector3f &other) : Eigen::Vector3f(other) {}
        template <class... Types>
        RGBf(const Eigen::CwiseBinaryOp<Types...> &op) : Eigen::Vector3f(op.matrix()) {}
        RGBf(const std::initializer_list<float> &other) : Eigen::Vector3f({other}) {}
        RGBf(float R, float G, float B) : Eigen::Vector3f(R, G, B) {}

        inline auto R() const -> const float & { return x(); }
        inline auto R() -> float & { return x(); }
        inline auto G() const -> const float & { return y(); }
        inline auto G() -> float & { return y(); }
        inline auto B() const -> const float & { return z(); }
        inline auto B() -> float & { return z(); }

        static const RGBf Min;
        static const RGBf Max;

        /// @brief RGB color from raw 24-bit RGB888 data
        static auto fromRGB888(const uint8_t *rgb888) -> RGBf;

        /// @brief RGB color from raw 32-bit XRGB888 data
        static auto fromXRGB888(uint32_t xrgb888) -> RGBf;

        /// @brief RGB color from raw RGB555 uint16_t
        static auto fromRGB555(uint16_t color) -> RGBf;

        /// @brief Convert color to raw RGB555 uint16_t by truncating and clamping
        auto toRGB555() const -> uint16_t;

        /// @brief Round and clamp RGB values to RGB555 grid positions. The values themselves will stay in [0,1]
        static auto roundToRGB555(const RGBf &color) -> RGBf;

        /// @brief Calculate square of perceived distance between colors
        /// See: https://stackoverflow.com/a/40950076 and https://www.compuphase.com/cmetric.htm
        /// @return Returns a value in [0,1]
        static auto distance(const RGBf &color0, const RGBf &color1) -> float;

        /// @brief Calculate square of distance between colors (scalar product)
        /// @return Returns block color distance in [0,1]
        template <std::size_t N>
        friend auto distance(const std::array<RGBf, N> &colors0, const std::array<RGBf, N> &colors1) -> float
        {
            float dist = 0.0F;
            for (auto c0It = colors0.cbegin(), c1It = colors1.cbegin(); c0It != colors0.cend() && c1It != colors1.cend(); ++c0It, ++c1It)
            {
                dist += distance(*c0It, *c1It);
            }
            return dist / (N * N);
        }
    };

}
