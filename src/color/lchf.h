#pragma once

#include "colorformat.h"

#include <Eigen/Eigen>
#include <Eigen/Core>

#include <array>
#include <cstdint>

namespace Color
{

    /// @brief Linear floating point CIE LCh / LCh(ab) color in range (NOT HCL or HSL)
    /// L [0,100] Luma
    /// C [0,200] Chroma (theoretically chroma is unbounded in LCh)
    /// h [0,360] Hue
    class LChf : public Eigen::Vector3f
    {
    public:
        using pixel_type = Eigen::Vector3f; // pixel value type
        using value_type = float;           // color channel value type
        static constexpr Color::Format ColorFormat = Format::LChf;
        static constexpr uint32_t Channels = pixel_type::RowsAtCompileTime;

        LChf() : pixel_type(0.0F, 0.0F, 0.0F) {}
        template <class... Types>
        LChf(const Eigen::CwiseBinaryOp<Types...> &op) : pixel_type(op.matrix()) {}
        LChf(const std::array<value_type, 3> &other) : pixel_type(other[0], other[1], other[2]) {}
        LChf(float L, float C, float H) : pixel_type(L, C, H) {}

        inline auto L() const -> const value_type & { return x(); }
        inline auto L() -> value_type & { return x(); }
        inline auto C() const -> const value_type & { return y(); }
        inline auto C() -> value_type & { return y(); }
        inline auto H() const -> const value_type & { return z(); }
        inline auto H() -> value_type & { return z(); }

        inline auto raw() const -> pixel_type { return *this; }

        static constexpr std::array<value_type, 3> Min{0.0F, 0.0F, 0.0F};
        static constexpr std::array<value_type, 3> Max{100.0F, 200.0F, 360.0F};

        /// @brief Simple euclidian distance color metric (CIE76). Ideally we would use DistanceCIEDE2000, but it is too expensive and complicated...
        /// See: https://en.wikipedia.org/wiki/Color_difference
        /// @return Returns a value in [0,1]
        static auto distance(const LChf &color0, const LChf &color1) -> float;
    };

}
