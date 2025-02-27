#pragma once

#include "blockview.h"
#include "color/conversions.h"
#include "color/psnr.h"

#include <array>
#include <memory>
#include <optional>
#include <type_traits>
#include <variant>
#include <vector>

using namespace Color;

/// @brief List of code book entries representing and pixels at block dimension 16x16, 8x8, 4x4
/// BLOCK_MAX_DIM must be > BLOCK_MIN_DIM and both must be a power-of-two. A maximum of 3 levels is allowed!
template <typename COLOR_TYPE, std::size_t BLOCK_MAX_DIM = 8, std::size_t BLOCK_MIN_DIM = 4>
class CodeBook
{
public:
    using value_type = COLOR_TYPE;
    using state_type = bool;
    static constexpr std::size_t BlockMaxDim = BLOCK_MAX_DIM;
    static constexpr std::size_t BlockMinDim = BLOCK_MIN_DIM;
    static constexpr std::size_t BlockLevels = std::log2(BlockMaxDim) - std::log2(BlockMinDim) + 1;
    static constexpr bool HasBlockLevel2 = BlockMaxDim / 4 >= BlockMinDim;
    using block_type = BlockView<value_type, state_type, BlockMaxDim, BlockMinDim>;

    CodeBook() = default;

    /// @brief Construct a codebook from pixel data
    template <typename T>
    CodeBook(const std::vector<T> &pixels, uint32_t width, uint32_t height, bool encoded = false)
        : m_width(width), m_height(height)
    {
        REQUIRE(pixels.size() == width * height, std::runtime_error, "Pixel data size must be same as width * height");
        REQUIRE(width % BlockMaxDim == 0, std::runtime_error, "Width must be a multiple of " << BlockMaxDim);
        REQUIRE(height % BlockMaxDim == 0, std::runtime_error, "Height must be a multiple of " << BlockMaxDim);
        if constexpr (std::is_same<T, COLOR_TYPE>())
        {
            m_pixels = pixels;
        }
        else
        {
            std::transform(pixels.cbegin(), pixels.cend(), std::back_inserter(m_pixels), [](const auto &pixel)
                           { return convertTo<COLOR_TYPE>(pixel); });
        }
        for (uint32_t y = 0; y < m_height; y += BlockMaxDim)
        {
            for (uint32_t x = 0; x < m_width; x += BlockMaxDim)
            {
                m_blocks.emplace_back(block_type(m_pixels.data(), m_width, m_height, x, y));
            }
        }
    }

    auto width() const
    {
        return m_width;
    }

    auto height() const
    {
        return m_height;
    }

    /// @brief Width of codebook in blocks
    template <std::size_t BLOCK_DIM = BlockMaxDim>
    auto blockWidth() const
    {
        // Block size must be inside allowed range of power-of-two
        static_assert(BLOCK_DIM <= BLOCK_MAX_DIM && BLOCK_DIM >= BLOCK_MIN_DIM && (BLOCK_DIM & (BLOCK_DIM - 1)) == 0);
        return m_width / BLOCK_DIM;
    }

    /// @brief Height of codebook in blocks
    template <std::size_t BLOCK_DIM = BlockMaxDim>
    auto blockHeight() const
    {
        // Block size must be inside allowed range of power-of-two
        static_assert(BLOCK_DIM <= BLOCK_MAX_DIM && BLOCK_DIM >= BLOCK_MIN_DIM && (BLOCK_DIM & (BLOCK_DIM - 1)) == 0);
        return m_height / BLOCK_DIM;
    }

    /// @brief Get code book blocks on highest level / block size
    const auto &blocks() const
    {
        return m_blocks;
    }

    /// @brief Get code book blocks on highest level / block size
    auto &blocks()
    {
        return m_blocks;
    }

    /// @brief Get code book block on highest level / block size
    const auto &block(std::size_t index) const
    {
        return m_blocks.at(index);
    }

    /// @brief Get code book block on highest level / block size
    auto &block(std::size_t index)
    {
        return m_blocks.at(index);
    }

    /// @brief Check if codebook has blocks
    auto empty() const -> bool
    {
        return m_blocks.empty();
    }

    /// @brief Get number of codebook blocks at specific level
    auto size() const
    {
        return m_blocks.size();
    }

    /// @brief Get codebook pixel data at full resolution
    auto pixels() const -> const std::vector<COLOR_TYPE> &
    {
        return m_pixels;
    }

    /// @brief Convert a codebook pixel data to other type
    template <typename T>
    auto convertPixels() const -> std::vector<T>
    {
        if constexpr (std::is_same<T, COLOR_TYPE>())
        {
            return m_pixels;
        }
        else
        {
            std::vector<T> pixels;
            std::transform(m_pixels.cbegin(), m_pixels.cend(), std::back_inserter(pixels), [](const auto &color)
                           { return convertTo<T>(color); });
            return pixels;
        }
    }

    /// @brief Calculate perceived pixel difference between codebooks
    auto mse(const CodeBook &b) -> float
    {
        return Color::mse(m_pixels, b.pixels());
    }

private:
    uint32_t m_width = 0;
    uint32_t m_height = 0;
    std::vector<COLOR_TYPE> m_pixels;
    std::vector<block_type> m_blocks;
};
