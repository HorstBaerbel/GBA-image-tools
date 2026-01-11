#pragma once

#include "colorformat.h"

#include <Eigen/Eigen>
#include <Eigen/Core>

#include <array>
#include <cstdint>

namespace Color
{

    /// @brief Linear floating point CIEL*a*b* color in range
    /// L* [0,100] Luma
    /// a* [-128,127]
    /// b* [-128,127]
    /// theoretically these are unbounded, but this is enough to cover sRGB
    class CIELabf : public Eigen::Vector3f
    {
    public:
        using pixel_type = Eigen::Vector3f; // pixel value type
        using value_type = float;           // color channel value type
        static constexpr Color::Format ColorFormat = Format::CIELabf;
        static constexpr uint32_t Channels = pixel_type::RowsAtCompileTime;

        CIELabf() : pixel_type(0.0F, 0.0F, 0.0F) {}
        template <class... Types>
        CIELabf(const Eigen::CwiseBinaryOp<Types...> &op) : pixel_type(op.matrix()) {}
        CIELabf(const std::array<value_type, 3> &other) : pixel_type(other[0], other[1], other[2]) {}
        CIELabf(float L, float C, float H) : pixel_type(L, C, H) {}

        inline auto L() const -> const value_type & { return x(); }
        inline auto L() -> value_type & { return x(); }
        inline auto a() const -> const value_type & { return y(); }
        inline auto a() -> value_type & { return y(); }
        inline auto b() const -> const value_type & { return z(); }
        inline auto b() -> value_type & { return z(); }

        static constexpr std::array<value_type, 3> Min{0.0F, -128.0F, -128.0F};
        static constexpr std::array<value_type, 3> Max{100.0F, 127.0F, 127.0F};

        /// @brief Calculate mean squared error between colors usind HyAB distance
        /// See: https://en.wikipedia.org/wiki/Color_difference#Other_geometric_constructions and http://markfairchild.org/PDFs/PAP40.pdf
        /// @return Returns a value in [0,1]
        static auto mse(const CIELabf &color0, const CIELabf &color1) -> float;

        /// @brief Calculate distance between colors using CIEDE2000
        /// @return Returns a value in [0,~185]
        /// @note This is very expensive computation-wise
        static auto ciede2000(const CIELabf &color0, const CIELabf &color1) -> float;
    };

}

// Specialization of std::hash for using class in std::map
template <>
struct std::hash<Color::CIELabf>
{
    std::size_t operator()(const Color::CIELabf &c) const noexcept
    {
        // get highest 21 bits of floating point number
        uint64_t x = (static_cast<uint64_t>(std::bit_cast<std::uint32_t>(c.L())) & 0xFFFFF800) << 32;
        uint64_t y = (static_cast<uint64_t>(std::bit_cast<std::uint32_t>(c.a())) & 0xFFFFF800) << 11;
        uint64_t z = (static_cast<uint64_t>(std::bit_cast<std::uint32_t>(c.b())) & 0xFFFFF800) >> 11;
        return x | y | z;
    }
};

// Specialization of std::less for using class in std::map
template <>
struct std::less<Color::CIELabf>
{
    bool operator()(const Color::CIELabf &lhs, const Color::CIELabf &rhs) const noexcept
    {
        return std::hash<Color::CIELabf>{}(lhs) < std::hash<Color::CIELabf>{}(rhs);
    }
};
