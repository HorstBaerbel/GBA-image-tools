#pragma once

#include "exception.h"

#include <array>
#include <cmath>
#include <vector>

namespace Color
{

    /// @brief Calculate power signal noise ratio between colors
    /// @return Returns a value in [0,1]
    /// @note Perceptual weighting of colors depends on color format used
    /// See: https://en.wikipedia.org/wiki/Peak_signal-to-noise_ratio
    /// And: https://stackoverflow.com/questions/16264141/power-signal-noise-ratio-psnr-of-colored-jpeg-image
    template <typename T>
    static auto psnr(const std::vector<T> &colors0, const std::vector<T> &colors1) -> float
    {
        REQUIRE(colors0.size() == colors1.size(), std::runtime_error, "Data size must be the same");
        double dist = 0.0;
        for (auto c0It = colors0.cbegin(), c1It = colors1.cbegin(); c0It != colors0.cend() && c1It != colors1.cend(); ++c0It, ++c1It)
        {
            dist += T::mse(*c0It, *c1It);
        }
        const auto mse = dist / colors0.size(); // calculate mean squared error
        return static_cast<float>(10.0 * std::log10(1.0 / mse));
    }

    /// @brief Calculate power signal noise ratio between colors
    /// @return Returns a value in [0,1]
    /// @note Perceptual weighting of colors depends on color format used
    /// See: https://en.wikipedia.org/wiki/Peak_signal-to-noise_ratio
    /// And: https://stackoverflow.com/questions/16264141/power-signal-noise-ratio-psnr-of-colored-jpeg-image
    template <typename T, std::size_t N>
    static auto psnr(const std::array<T, N> &colors0, const std::array<T, N> &colors1) -> float
    {
        double dist = 0.0;
        for (auto c0It = colors0.cbegin(), c1It = colors1.cbegin(); c0It != colors0.cend() && c1It != colors1.cend(); ++c0It, ++c1It)
        {
            dist += T::mse(*c0It, *c1It);
        }
        const auto mse = dist / colors0.size(); // calculate mean squared error
        return static_cast<float>(10.0 * std::log10(1.0 / mse));
    }

    /// @brief Calculate mean squared error between pixels and block in image (scalar product)
    /// @return Returns average color distance in [0,1]
    template <typename T>
    static auto mse(const std::vector<T> &image, const uint32_t pixelsPerScanline, const std::vector<T> &block, const uint32_t blockX, const uint32_t blockY, const uint32_t blockWidth, const uint32_t blockHeight) -> float
    {
        REQUIRE(block.size() == blockWidth * blockHeight, std::runtime_error, "Data size must be the same as width * height");
        double dist = 0.0;
        auto iIt = std::next(image.cbegin(), blockY * pixelsPerScanline + blockX);
        auto bIt = block.cbegin();
        for (uint32_t y = 0; y < blockHeight; ++y)
        {
            for (uint32_t x = 0; x < blockWidth; ++x, ++iIt, ++bIt)
            {
                dist += T::mse(*iIt, *bIt);
            }
            iIt = std::next(iIt, pixelsPerScanline - blockWidth);
        }
        return static_cast<float>(dist / block.size());
    }

    /// @brief Calculate mean squared error between colors (scalar product)
    /// @return Returns average color distance in [0,1]
    template <typename T>
    static auto mse(const std::vector<T> &colors0, const std::vector<T> &colors1) -> float
    {
        REQUIRE(colors0.size() == colors1.size(), std::runtime_error, "Data size must be the same");
        double dist = 0.0;
        for (auto c0It = colors0.cbegin(), c1It = colors1.cbegin(); c0It != colors0.cend() && c1It != colors1.cend(); ++c0It, ++c1It)
        {
            dist += T::mse(*c0It, *c1It);
        }
        return static_cast<float>(dist / colors0.size());
    }

    /// @brief Calculate mean squared error between colors (scalar product)
    /// @return Returns average block color distance in [0,1]
    template <typename T, std::size_t N>
    static auto mse(const std::array<T, N> &colors0, const std::array<T, N> &colors1) -> float
    {
        double dist = 0.0;
        for (auto c0It = colors0.cbegin(), c1It = colors1.cbegin(); c0It != colors0.cend() && c1It != colors1.cend(); ++c0It, ++c1It)
        {
            dist += T::mse(*c0It, *c1It);
        }
        return static_cast<float>(dist / N);
    }

    /// @brief Calculate mean squared error between colors (scalar product) and if there are are outliers above a threshold
    /// @return Returns (true if all colors below threshold, average color distance in [0,1])
    template <typename T, std::size_t N>
    static auto mseBelowThreshold(const std::array<T, N> &colors0, const std::array<T, N> &colors1, float threshold) -> std::pair<bool, float>
    {
        bool belowThreshold = true;
        double dist = 0.0;
        for (auto c0It = colors0.cbegin(), c1It = colors1.cbegin(); c0It != colors0.cend() && c1It != colors1.cend(); ++c0It, ++c1It)
        {
            auto colorDist = T::mse(*c0It, *c1It);
            belowThreshold = belowThreshold ? colorDist < threshold : belowThreshold;
            dist += colorDist;
        }
        return {belowThreshold, static_cast<float>(dist / N)};
    }

}
