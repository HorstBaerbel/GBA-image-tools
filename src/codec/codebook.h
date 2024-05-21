#pragma once

#include "blockview.h"
#include "color/conversions.h"
#include "color/psnr.h"

#include <array>
#include <memory>
#include <optional>
#include <type_traits>
#include <vector>

using namespace Color;

/// @brief List of code book entries representing and image at block dimension 16x16, 8x8, 4x4
template <typename COLOR_TYPE>
class CodeBook
{
public:
    using value_type = COLOR_TYPE;
    using state_type = bool;
    static constexpr std::size_t BlockMaxDim = 16;
    static constexpr std::size_t BlockMinDim = 4;
    static constexpr std::size_t BlockLevels = 4 - 2; // std::log2(BlockMaxDim) - std::log2(BlockMinDim);
    using block_type0 = BlockView<value_type, BlockMaxDim, BlockMinDim>;
    using block_type1 = BlockView<value_type, BlockMaxDim / 2, BlockMinDim>;
    using block_type2 = BlockView<value_type, BlockMaxDim / 4, BlockMinDim>;

    CodeBook() = default;

    /// @brief Construct a codebook from image data
    template <typename T>
    CodeBook(const std::vector<T> &image, uint32_t width, uint32_t height, bool encoded = false)
        : m_width(width), m_height(height)
    {
        std::transform(image.cbegin(), image.cend(), std::back_inserter(m_colors), [](const auto &pixel)
                       { return convertTo<COLOR_TYPE>(pixel); });
        for (uint32_t y = 0; y < m_height; y += BlockMaxDim)
        {
            for (uint32_t x = 0; x < m_width; x += BlockMaxDim)
            {
                m_blocks0.emplace_back(block_type0(m_colors.data(), m_width, m_height, x, y));
            }
        }
        for (uint32_t y = 0; y < m_height; y += BlockMaxDim / 2)
        {
            for (uint32_t x = 0; x < m_width; x += BlockMaxDim / 2)
            {
                m_blocks1.emplace_back(block_type1(m_colors.data(), m_width, m_height, x, y));
            }
        }
        for (uint32_t y = 0; y < m_height; y += BlockMaxDim / 4)
        {
            for (uint32_t x = 0; x < m_width; x += BlockMaxDim / 4)
            {
                m_blocks2.emplace_back(block_type2(m_colors.data(), m_width, m_height, x, y));
            }
        }
        m_encoded0 = std::vector<bool>(m_width / BlockMaxDim * m_height / BlockMaxDim, encoded);
        m_encoded1 = std::vector<bool>(m_width / (BlockMaxDim / 2) * m_height / (BlockMaxDim / 2), encoded);
        m_encoded2 = std::vector<bool>(m_width / (BlockMaxDim / 4) * m_height / (BlockMaxDim / 4), encoded);
    }

    /// @brief Block iterator to start of blocks
    template <std::size_t BLOCK_DIM>
    auto begin() noexcept
    {
        if constexpr (BLOCK_DIM == decltype(m_blocks0)::value_type::Dim)
        {
            return m_blocks0.begin();
        }
        else if constexpr (BLOCK_DIM == decltype(m_blocks1)::value_type::Dim)
        {
            return m_blocks1.begin();
        }
        else if constexpr (BLOCK_DIM == decltype(m_blocks2)::value_type::Dim)
        {
            return m_blocks2.begin();
        }
    }

    /// @brief Block iterator past end of blocks
    template <std::size_t BLOCK_DIM>
    auto end() noexcept
    {
        if constexpr (BLOCK_DIM == decltype(m_blocks0)::value_type::Dim)
        {
            return m_blocks0.end();
        }
        else if constexpr (BLOCK_DIM == decltype(m_blocks1)::value_type::Dim)
        {
            return m_blocks1.end();
        }
        else if constexpr (BLOCK_DIM == decltype(m_blocks2)::value_type::Dim)
        {
            return m_blocks2.end();
        }
    }

    /// @brief Block iterator to start of blocks
    template <std::size_t BLOCK_DIM>
    auto cbegin() const noexcept
    {
        if constexpr (BLOCK_DIM == decltype(m_blocks0)::value_type::Dim)
        {
            return m_blocks0.cbegin();
        }
        else if constexpr (BLOCK_DIM == decltype(m_blocks1)::value_type::Dim)
        {
            return m_blocks1.cbegin();
        }
        else if constexpr (BLOCK_DIM == decltype(m_blocks2)::value_type::Dim)
        {
            return m_blocks2.cbegin();
        }
    }

    /// @brief Block iterator past end of blocks
    template <std::size_t BLOCK_DIM>
    auto cend() const noexcept
    {
        if constexpr (BLOCK_DIM == decltype(m_blocks0)::value_type::Dim)
        {
            return m_blocks0.cend();
        }
        else if constexpr (BLOCK_DIM == decltype(m_blocks1)::value_type::Dim)
        {
            return m_blocks1.cend();
        }
        else if constexpr (BLOCK_DIM == decltype(m_blocks2)::value_type::Dim)
        {
            return m_blocks2.cend();
        }
    }

    /// @brief Check if codebook has blocks
    template <std::size_t BLOCK_DIM>
    auto empty() const -> bool
    {
        if constexpr (BLOCK_DIM == decltype(m_blocks0)::value_type::Dim)
        {
            return m_blocks0.empty();
        }
        else if constexpr (BLOCK_DIM == decltype(m_blocks1)::value_type::Dim)
        {
            return m_blocks1.empty();
        }
        else if constexpr (BLOCK_DIM == decltype(m_blocks2)::value_type::Dim)
        {
            return m_blocks2.empty();
        }
    }

    /// @brief Get number codebook blocks at specific level
    template <std::size_t BLOCK_DIM>
    auto size() const
    {
        if constexpr (BLOCK_DIM == decltype(m_blocks0)::value_type::Dim)
        {
            return m_blocks0.size();
        }
        else if constexpr (BLOCK_DIM == decltype(m_blocks1)::value_type::Dim)
        {
            return m_blocks1.size();
        }
        else if constexpr (BLOCK_DIM == decltype(m_blocks2)::value_type::Dim)
        {
            return m_blocks2.size();
        }
    }

    template <std::size_t BLOCK_DIM>
    auto isEncoded(const BlockView<COLOR_TYPE, BLOCK_DIM> &block) const
    {
        if constexpr (BLOCK_DIM == decltype(m_blocks0)::value_type::Dim)
        {
            return m_encoded0[block.index()];
        }
        else if constexpr (BLOCK_DIM == decltype(m_blocks1)::value_type::Dim)
        {
            return m_encoded1[block.index()];
        }
        else if constexpr (BLOCK_DIM == decltype(m_blocks2)::value_type::Dim)
        {
            return m_encoded2[block.index()];
        }
    }

    template <std::size_t BLOCK_DIM>
    auto setEncoded(const BlockView<COLOR_TYPE, BLOCK_DIM> &block, bool encoded = true)
    {
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
            setEncoded<BLOCK_DIM / 2>(block.block(0), encoded);
            setEncoded<BLOCK_DIM / 2>(block.block(1), encoded);
            setEncoded<BLOCK_DIM / 2>(block.block(2), encoded);
            setEncoded<BLOCK_DIM / 2>(block.block(3), encoded);
        }
        else if constexpr (BLOCK_DIM == decltype(m_blocks2)::value_type::Dim)
        {
            m_encoded2[index] = encoded;
        }
    }

    /// @brief Convert a codebook to image data
    template <typename T>
    auto toImage() const -> std::vector<T>
    {
        std::vector<T> image;
        std::transform(m_colors.cbegin(), m_colors.cend(), std::back_inserter(image), [](const auto &color)
                       { return convertTo<T>(color); });
        return image;
    }

    /// @brief Calculate perceived pixel difference between codebooks
    auto distance(const CodeBook &b) -> float
    {
        float sum = 0.0F;
        auto aIt = m_colors.cbegin();
        auto bIt = b.m_colors.cbegin();
        while (aIt != m_colors.cend() && bIt != b.m_colors.cend())
        {
            sum += COLOR_TYPE::mse(*aIt++, *bIt++);
        }
        return sum / m_blocks2.size();
    }

private:
    uint32_t m_width = 0;
    uint32_t m_height = 0;
    std::vector<COLOR_TYPE> m_colors;
    std::vector<block_type0> m_blocks0;
    std::vector<block_type1> m_blocks1;
    std::vector<block_type2> m_blocks2;
    std::vector<bool> m_encoded0;
    std::vector<bool> m_encoded1;
    std::vector<bool> m_encoded2;
};
