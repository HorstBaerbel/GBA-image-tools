#pragma once

#include <Eigen/Core>

#include <array>
#include <cmath>

namespace DCT
{

    /// @brief DCT-II function for N x N blocks of values
    /// @tparam N Columns of value block
    /// @tparam M Rows of value block
    /// @tparam R Return array type
    /// @tparam T Value or struct type
    /// @tparam A Accessor function type to get an R from T, e.g. to get a value from an RGB color
    /// This must be a function taking T and converting it to R
    /// See: https://stackoverflow.com/questions/8310749/discrete-cosine-transform-dct-implementation-c
    /// And: https://en.wikipedia.org/wiki/Discrete_cosine_transform#DCT-II
    template <std::size_t N, std::size_t M, typename R, typename T, typename A>
    auto dct(const std::array<T, N * M> &values, A accessor) -> std::array<R, N * M>
    {
        constexpr auto NR = static_cast<R>(N);
        constexpr auto MR = static_cast<R>(M);
        constexpr auto PI_N = M_PI / NR;
        constexpr auto PI_M = M_PI / MR;
        constexpr auto HALF = R(0.5);
        std::array<R, N * M> result{};
        for (int64_t u = 0; u < N; ++u)
        {
            for (int64_t v = 0; v < M; ++v)
            {
                auto &uvResult = result[u * N + v];
                uvResult = 0;
                for (int64_t i = 0; i < N; i++)
                {
                    const auto uiFactor = std::cos(PI_N * (static_cast<R>(i) + HALF) * static_cast<R>(u));
                    for (int64_t j = 0; j < M; j++)
                    {
                        uvResult += values[i * N + j] * uiFactor * std::cos(PI_M * (static_cast<R>(j) + HALF) * static_cast<R>(v));
                    }
                }
            }
        }
        return result;
    }

    /// @brief DCT-II function for N x N blocks of vectors
    /// @tparam N Columns of value block
    /// @tparam M Rows of value block
    /// @tparam T Vector scalar type
    /// See: https://stackoverflow.com/questions/8310749/discrete-cosine-transform-dct-implementation-c
    /// And: https://en.wikipedia.org/wiki/Discrete_cosine_transform#DCT-II
    template <std::size_t N, std::size_t M, typename T>
    auto dct(const std::array<T, N * M> &values) -> std::array<T, N * M>
    {
        using R = typename T::Scalar;
        constexpr auto NR = static_cast<R>(N);
        constexpr auto MR = static_cast<R>(M);
        constexpr auto PI_N = M_PI / NR;
        constexpr auto PI_M = M_PI / MR;
        constexpr auto HALF = R(0.5);
        std::array<T, N * M> result{};
        for (int64_t row = 0; row < T::RowsAtCompileTime; ++row)
        {
            for (int64_t u = 0; u < N; ++u)
            {
                for (int64_t v = 0; v < M; ++v)
                {
                    auto &uvResult = result[u * N + v][row];
                    uvResult = 0;
                    for (int64_t i = 0; i < N; i++)
                    {
                        const auto uiFactor = std::cos(PI_N * (static_cast<R>(i) + HALF) * static_cast<R>(u));
                        for (int64_t j = 0; j < M; j++)
                        {
                            uvResult += values[i * N + j][row] * uiFactor * std::cos(PI_M * (static_cast<R>(j) + HALF) * static_cast<R>(v));
                        }
                    }
                }
            }
        }
        return result;
    }

}
