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
    using block_type0 = BlockView<value_type, BlockMaxDim, BlockMinDim>;
    using block_type1 = BlockView<value_type, BlockMaxDim / 2, BlockMinDim>;
    using block_type2 = BlockView<value_type, BlockMaxDim / 4, BlockMinDim>;

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
                m_blocks0.emplace_back(block_type0(m_pixels.data(), m_width, m_height, x, y));
            }
        }
        m_encoded0 = std::vector<bool>(m_width / BlockMaxDim * m_height / BlockMaxDim, encoded);
        for (uint32_t y = 0; y < m_height; y += BlockMaxDim / 2)
        {
            for (uint32_t x = 0; x < m_width; x += BlockMaxDim / 2)
            {
                m_blocks1.emplace_back(block_type1(m_pixels.data(), m_width, m_height, x, y));
            }
        }
        m_encoded1 = std::vector<bool>(m_width / (BlockMaxDim / 2) * m_height / (BlockMaxDim / 2), encoded);
        if constexpr (HasBlockLevel2)
        {
            for (uint32_t y = 0; y < m_height; y += BlockMaxDim / 4)
            {
                for (uint32_t x = 0; x < m_width; x += BlockMaxDim / 4)
                {
                    m_blocks2.emplace_back(block_type2(m_pixels.data(), m_width, m_height, x, y));
                }
            }
            m_encoded2 = std::vector<bool>(m_width / (BlockMaxDim / 4) * m_height / (BlockMaxDim / 4), encoded);
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

    /// @brief Block iterator to start of blocks
    template <std::size_t BLOCK_DIM = BlockMaxDim>
    auto begin() noexcept
    {
        // Block size must be inside allowed range of power-of-two
        static_assert(BLOCK_DIM <= BLOCK_MAX_DIM && BLOCK_DIM >= BLOCK_MIN_DIM && (BLOCK_DIM & (BLOCK_DIM - 1)) == 0);
        if constexpr (BLOCK_DIM == decltype(m_blocks0)::value_type::Dim)
        {
            return m_blocks0.begin();
        }
        else if constexpr (BLOCK_DIM == decltype(m_blocks1)::value_type::Dim)
        {
            return m_blocks1.begin();
        }
        else if constexpr (HasBlockLevel2 && BLOCK_DIM == decltype(m_blocks2)::value_type::Dim)
        {
            return m_blocks2.begin();
        }
    }

    /// @brief Block iterator past end of blocks
    template <std::size_t BLOCK_DIM = BlockMaxDim>
    auto end() noexcept
    {
        // Block size must be inside allowed range of power-of-two
        static_assert(BLOCK_DIM <= BLOCK_MAX_DIM && BLOCK_DIM >= BLOCK_MIN_DIM && (BLOCK_DIM & (BLOCK_DIM - 1)) == 0);
        if constexpr (BLOCK_DIM == decltype(m_blocks0)::value_type::Dim)
        {
            return m_blocks0.end();
        }
        else if constexpr (BLOCK_DIM == decltype(m_blocks1)::value_type::Dim)
        {
            return m_blocks1.end();
        }
        else if constexpr (HasBlockLevel2 && BLOCK_DIM == decltype(m_blocks2)::value_type::Dim)
        {
            return m_blocks2.end();
        }
    }

    /// @brief Block iterator to start of blocks
    template <std::size_t BLOCK_DIM = BlockMaxDim>
    auto cbegin() const noexcept
    {
        // Block size must be inside allowed range of power-of-two
        static_assert(BLOCK_DIM <= BLOCK_MAX_DIM && BLOCK_DIM >= BLOCK_MIN_DIM && (BLOCK_DIM & (BLOCK_DIM - 1)) == 0);
        if constexpr (BLOCK_DIM == decltype(m_blocks0)::value_type::Dim)
        {
            return m_blocks0.cbegin();
        }
        else if constexpr (BLOCK_DIM == decltype(m_blocks1)::value_type::Dim)
        {
            return m_blocks1.cbegin();
        }
        else if constexpr (HasBlockLevel2 && BLOCK_DIM == decltype(m_blocks2)::value_type::Dim)
        {
            return m_blocks2.cbegin();
        }
    }

    /// @brief Block iterator past end of blocks
    template <std::size_t BLOCK_DIM = BlockMaxDim>
    auto cend() const noexcept
    {
        // Block size must be inside allowed range of power-of-two
        static_assert(BLOCK_DIM <= BLOCK_MAX_DIM && BLOCK_DIM >= BLOCK_MIN_DIM && (BLOCK_DIM & (BLOCK_DIM - 1)) == 0);
        if constexpr (BLOCK_DIM == decltype(m_blocks0)::value_type::Dim)
        {
            return m_blocks0.cend();
        }
        else if constexpr (BLOCK_DIM == decltype(m_blocks1)::value_type::Dim)
        {
            return m_blocks1.cend();
        }
        else if constexpr (HasBlockLevel2 && BLOCK_DIM == decltype(m_blocks2)::value_type::Dim)
        {
            return m_blocks2.cend();
        }
    }

    /// @brief Check if codebook has blocks
    template <std::size_t BLOCK_DIM = BlockMaxDim>
    auto empty() const -> bool
    {
        // Block size must be inside allowed range of power-of-two
        static_assert(BLOCK_DIM <= BLOCK_MAX_DIM && BLOCK_DIM >= BLOCK_MIN_DIM && (BLOCK_DIM & (BLOCK_DIM - 1)) == 0);
        if constexpr (BLOCK_DIM == decltype(m_blocks0)::value_type::Dim)
        {
            return m_blocks0.empty();
        }
        else if constexpr (BLOCK_DIM == decltype(m_blocks1)::value_type::Dim)
        {
            return m_blocks1.empty();
        }
        else if constexpr (HasBlockLevel2 && BLOCK_DIM == decltype(m_blocks2)::value_type::Dim)
        {
            return m_blocks2.empty();
        }
    }

    /// @brief Get number of codebook blocks at specific level
    template <std::size_t BLOCK_DIM = BlockMaxDim>
    auto size() const
    {
        // Block size must be inside allowed range of power-of-two
        static_assert(BLOCK_DIM <= BLOCK_MAX_DIM && BLOCK_DIM >= BLOCK_MIN_DIM && (BLOCK_DIM & (BLOCK_DIM - 1)) == 0);
        if constexpr (BLOCK_DIM == decltype(m_blocks0)::value_type::Dim)
        {
            return m_blocks0.size();
        }
        else if constexpr (BLOCK_DIM == decltype(m_blocks1)::value_type::Dim)
        {
            return m_blocks1.size();
        }
        else if constexpr (HasBlockLevel2 && BLOCK_DIM == decltype(m_blocks2)::value_type::Dim)
        {
            return m_blocks2.size();
        }
    }

    template <std::size_t BLOCK_DIM = BlockMaxDim>
    auto isEncoded(const BlockView<COLOR_TYPE, BLOCK_DIM> &block) const
    {
        // Block size must be inside allowed range of power-of-two
        static_assert(BLOCK_DIM <= BLOCK_MAX_DIM && BLOCK_DIM >= BLOCK_MIN_DIM && (BLOCK_DIM & (BLOCK_DIM - 1)) == 0);
        if constexpr (BLOCK_DIM == decltype(m_blocks0)::value_type::Dim)
        {
            return m_encoded0[block.index()];
        }
        else if constexpr (BLOCK_DIM == decltype(m_blocks1)::value_type::Dim)
        {
            return m_encoded1[block.index()];
        }
        else if constexpr (HasBlockLevel2 && BLOCK_DIM == decltype(m_blocks2)::value_type::Dim)
        {
            return m_encoded2[block.index()];
        }
    }

    template <std::size_t BLOCK_DIM = BlockMaxDim>
    auto setEncoded(const BlockView<COLOR_TYPE, BLOCK_DIM> &block, bool encoded = true)
    {
        // Block size must be inside allowed range of power-of-two
        static_assert(BLOCK_DIM <= BLOCK_MAX_DIM && BLOCK_DIM >= BLOCK_MIN_DIM && (BLOCK_DIM & (BLOCK_DIM - 1)) == 0);
        auto index = block.index();
        if constexpr (BLOCK_DIM == decltype(m_blocks0)::value_type::Dim)
        {
            m_encoded0[index] = encoded;
            setEncoded<BLOCK_DIM / 2>(block.block(0), encoded);
            setEncoded<BLOCK_DIM / 2>(block.block(1), encoded);
            setEncoded<BLOCK_DIM / 2>(block.block(2), encoded);
            setEncoded<BLOCK_DIM / 2>(block.block(3), encoded);
        }
        else if constexpr (BLOCK_DIM == decltype(m_blocks1)::value_type::Dim)
        {
            m_encoded1[index] = encoded;
            if constexpr (HasBlockLevel2)
            {
                setEncoded<BLOCK_DIM / 2>(block.block(0), encoded);
                setEncoded<BLOCK_DIM / 2>(block.block(1), encoded);
                setEncoded<BLOCK_DIM / 2>(block.block(2), encoded);
                setEncoded<BLOCK_DIM / 2>(block.block(3), encoded);
            }
        }
        else if constexpr (HasBlockLevel2 && BLOCK_DIM == decltype(m_blocks2)::value_type::Dim)
        {
            m_encoded2[index] = encoded;
        }
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
        double sum = 0.0;
        auto aIt = m_pixels.cbegin();
        auto bIt = b.m_pixels.cbegin();
        while (aIt != m_pixels.cend() && bIt != b.m_pixels.cend())
        {
            sum += COLOR_TYPE::mse(*aIt++, *bIt++);
        }
        if constexpr (HasBlockLevel2)
        {
            return static_cast<float>(sum / m_blocks2.size());
        }
        else
        {
            return static_cast<float>(sum / m_blocks1.size());
        }
    }

private:
    uint32_t m_width = 0;
    uint32_t m_height = 0;
    std::vector<COLOR_TYPE> m_pixels;
    std::vector<block_type0> m_blocks0;
    std::vector<block_type1> m_blocks1;
    [[no_unique_address]] std::conditional_t<HasBlockLevel2, std::vector<block_type2>, std::monostate> m_blocks2;
    std::vector<bool> m_encoded0;
    std::vector<bool> m_encoded1;
    [[no_unique_address]] std::conditional_t<HasBlockLevel2, std::vector<bool>, std::monostate> m_encoded2;
};
