#pragma once

#include <Eigen/Core>

#include <array>
#include <cmath>

namespace Correlation
{

    /// @brief 1D normalized cross-correlate of arrays at zero shift with Eigen::Vectors
    /// @tparam T Array type
    /// @tparam N Number of values in array
    /// @return Correlation value in [-1,1]
    /// See: http://paulbourke.net/miscellaneous/correlate/
    /// See: https://en.wikipedia.org/wiki/Cross-correlation
    template <typename T, std::size_t N>
    auto crosscorrelate0(const std::array<T, N> &a, const std::array<T, N> &b) -> T
    {
        // calculate the mean of a and b
        T meanA{};
        T meanB{};
        for (std::size_t i = 0; i < N; ++i)
        {
            meanA += a[i];
            meanB += b[i];
        }
        meanA /= N;
        meanB /= N;
        // calculate the denominator
        T sA{};
        T sB{};
        for (std::size_t i = 0; i < N; ++i)
        {
            sA += (a[i] - meanA).cwiseProduct(a[i] - meanA);
            sB += (b[i] - meanB).cwiseProduct(b[i] - meanB);
        }
        T denom{};
        for (std::size_t row = 0; row < T::RowsAtCompileTime; ++row)
        {
            denom[row] = std::sqrt(sA[row] * sB[row]);
        }
        // calculate the 0-shift correlation
        T sAB{};
        for (std::size_t i = 0; i < N; ++i)
        {
            sAB += (a[i] - meanA).cwiseProduct(b[i] - meanB);
        }
        // calculate correlation value [-1,1]
        auto r = sAB.cwiseQuotient(denom);
        return r;
    }

    /// @brief 1D convolution of arrays at zero shift with Eigen::Vectors
    /// @tparam T Array type
    /// @tparam N Number of values in array
    /// @return Correlation value in [-1,1]
    /// See: http://paulbourke.net/miscellaneous/correlate/
    /// See: https://en.wikipedia.org/wiki/Cross-correlation
    template <typename T, std::size_t N>
    auto convolve0(const std::array<T, N> &a, const std::array<T, N> &b) -> T
    {
        // calculate the 0-shift convolution
        T sAB{};
        for (std::size_t i = 0; i < N; ++i)
        {
            sAB += a[i].cwiseProduct(b[i]);
        }
        return sAB;
    }

}