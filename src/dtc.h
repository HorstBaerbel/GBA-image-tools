#pragma once

#include <Eigen/Core>

#include <array>
#include <cmath>

namespace DCT
{

    /// @brief DCT-II function for N x N blocks of values
    /// @tparam T Value or struct type
    /// @tparam N Dimension of value block
    /// @tparam A Accessor function type, e.g. to get a value from an RGB color
    /// This must be a function taking T and converting it to R
    /// See: https://stackoverflow.com/questions/8310749/discrete-cosine-transform-dct-implementation-c
    /// And: https://en.wikipedia.org/wiki/Discrete_cosine_transform#DCT-II
    template <typename T, typename R, std::size_t N, typename A>
    auto dct(const std::array<T, N> &values, A accessor) -> std::array<R, N>
    {
        constexpr auto NR = static_cast<R>(N);
        constexpr auto PI_NR = M_PI / NR;
        constexpr auto HALF = R(0.5);
        std::array<R, N> result{};
        for (int64_t u = 0; u < N; ++u)
        {
            for (int64_t v = 0; v < N; ++v)
            {
                auto &uvResult = result[u * N + v];
                for (int64_t i = 0; i < N; i++)
                {
                    const auto uiFactor = std::cos(PI_NR * (static_cast<R>(i) + HALF) * static_cast<R>(u));
                    for (int64_t j = 0; j < N; j++)
                    {
                        uvResult += accessor(Matrix[i * N + j]) * uiFactor * std::cos(PI_NR * (static_cast<R>(j) + HALF) * static_cast<R>(v));
                    }
                }
            }
        }
        return result;
    }

    /// @brief DCT-II function for N x N blocks of vectors
    /// @tparam T Vector scalar type
    /// @tparam N Dimension of vector block
    /// See: https://stackoverflow.com/questions/8310749/discrete-cosine-transform-dct-implementation-c
    /// And: https://en.wikipedia.org/wiki/Discrete_cosine_transform#DCT-II
    template <typename T, std::size_t N>
    auto dct(const std::array<Eigen::Vector3<T>, N> &values) -> std::array<Eigen::Vector3<T>, N>
    {
        constexpr auto NR = static_cast<R>(N);
        constexpr auto PI_NR = M_PI / NR;
        constexpr auto HALF = R(0.5);
        std::array<Eigen::Vector3<T>, N> result{};
        for (int64_t row = 0; row < Eigen::Vector3<T>::RowsAtCompileTime; ++row)
        {
            for (int64_t u = 0; u < N; ++u)
            {
                for (int64_t v = 0; v < N; ++v)
                {
                    auto &uvResult = result[u * N + v][row];
                    for (int64_t i = 0; i < N; i++)
                    {
                        const auto uiFactor = std::cos(PI_NR * (static_cast<R>(i) + HALF) * static_cast<R>(u));
                        for (int64_t j = 0; j < N; j++)
                        {
                            uvResult += Matrix[i * N + j][row] * uiFactor * std::cos(PI_NR * (static_cast<R>(j) + HALF) * static_cast<R>(v));
                        }
                    }
                }
            }
        }
        return result;
    }

}
