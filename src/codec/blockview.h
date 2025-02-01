#pragma once

#include "exception.h"

#include <array>
#include <cstdint>
#include <memory>
#include <vector>

template <typename T>
struct ViewIterator
{
    using iterator_category = std::forward_iterator_tag;
    using difference_type = std::ptrdiff_t;
    using value_type = T;
    using pointer = T *;   // or also value_type*
    using reference = T &; // or also value_type&

    ViewIterator(pointer values, const uint32_t *indices, std::size_t position) noexcept
        : m_values(values), m_indices(indices), m_position(position)
    {
    }

    reference operator*() const { return m_values[m_indices[m_position]]; }
    pointer operator->() { return &m_values[m_indices[m_position]]; }

    ViewIterator &operator++() noexcept
    {
        m_position++;
        return *this;
    }
    ViewIterator operator++(int) noexcept
    {
        ViewIterator tmp = *this;
        ++(*this);
        return tmp;
    }

    friend bool operator==(const ViewIterator &a, const ViewIterator &b) { return a.m_position == b.m_position; };
    friend bool operator!=(const ViewIterator &a, const ViewIterator &b) { return a.m_position != b.m_position; };

private:
    pointer m_values = nullptr;
    const uint32_t *m_indices = nullptr;
    std::size_t m_position = 0;
};

/// @brief Struct describing an N*N block of pixels that references part of an image.
/// It does not hold the color data itself, but merely references it.
template <typename T, std::size_t N, size_t MIN_DIM = 4>
class BlockView
{
};

template <typename T, std::size_t N>
class BlockView<T, N, 4>
{
public:
    using value_type = T;
    static constexpr std::size_t Dim = N;
    static constexpr std::size_t MinDim = 4;

    using Iterator = ViewIterator<value_type>;
    using ConstIterator = ViewIterator<const value_type>;

    BlockView() = default;

    BlockView(value_type *pixels, uint32_t width, uint32_t height, uint32_t x, uint32_t y)
        : m_pixels(pixels), m_width(width), m_height(height), m_x(x), m_y(y), m_blockIndex(m_y / Dim * m_width / Dim + m_x / Dim)
    {
        auto offset = m_y * m_width + m_x;
        auto index = 0;
        for (std::size_t j = 0; j < Dim; ++j)
        {
            for (std::size_t i = 0; i < Dim; ++i)
            {
                m_indices[index++] = offset + i;
            }
            offset += m_width;
        }
        if constexpr (Dim > MinDim)
        {
            m_subblocks[0] = BlockView<value_type, Dim / 2, MinDim>(m_pixels, m_width, m_height, x, y);
            m_subblocks[1] = BlockView<value_type, Dim / 2, MinDim>(m_pixels, m_width, m_height, x + Dim / 2, y);
            m_subblocks[2] = BlockView<value_type, Dim / 2, MinDim>(m_pixels, m_width, m_height, x, y + Dim / 2);
            m_subblocks[3] = BlockView<value_type, Dim / 2, MinDim>(m_pixels, m_width, m_height, x + Dim / 2, y + Dim / 2);
        }
    }

    BlockView &operator=(const std::array<value_type, Dim * Dim> &pixels)
    {
        for (std::size_t i = 0; i < Dim * Dim; ++i)
        {
            m_pixels[m_indices[i]] = pixels[i];
        }
        return *this;
    }

    /// @brief Return block index in image. Blocks are stored row-wise.
    /// Each block level has their own block indices
    auto index() const
    {
        return m_blockIndex;
    }

    /// @brief Return x-position
    auto x() const
    {
        return m_x;
    }

    /// @brief Return y-position
    auto y() const
    {
        return m_y;
    }

    Iterator begin() noexcept
    {
        return Iterator(m_pixels, m_indices.data(), 0);
    }

    Iterator end() noexcept
    {
        return Iterator(m_pixels, m_indices.data(), m_indices.size());
    }

    ConstIterator cbegin() const noexcept
    {
        return ConstIterator(m_pixels, m_indices.data(), 0);
    }

    ConstIterator cend() const noexcept
    {
        return ConstIterator(m_pixels, m_indices.data(), m_indices.size());
    }

    auto empty() const -> bool
    {
        return m_indices.size() == 0;
    }

    auto size() const -> std::size_t
    {
        return m_indices.size();
    }

    auto operator[](std::size_t index) const -> const value_type &
    {
        return m_pixels[m_indices[index]];
    }

    auto operator[](std::size_t index) -> value_type &
    {
        return m_pixels[m_indices[index]];
    }

    /// @brief Return block pixels as deep-copy compact array
    auto pixels() const -> std::vector<value_type>
    {
        std::vector<value_type> result(Dim * Dim);
        for (std::size_t i = 0; i < Dim * Dim; ++i)
        {
            result[i] = m_pixels[m_indices[i]];
        }
        return result;
    }

    /// @brief Deep copy pixels from other block into this one
    auto copyPixelsFrom(const BlockView &other) -> void
    {
        for (std::size_t i = 0; i < Dim * Dim; ++i)
        {
            m_pixels[m_indices[i]] = other.m_pixels[other.m_indices[i]];
        }
    }

    /// @brief Deep copy pixels from other block into this one
    auto copyPixelsFrom(const std::vector<value_type> &pixels) -> void
    {
        REQUIRE(pixels.size() == Dim * Dim, std::runtime_error, "Data must have same size as block");
        for (std::size_t i = 0; i < Dim * Dim; ++i)
        {
            m_pixels[m_indices[i]] = pixels[i];
        }
    }

    /// @brief Get sub-blocks of this one. Blocks are stored row-wise
    const auto &blocks() const
    {
        return m_subblocks;
    }

    /// @brief Get sub-blocks of this one. Blocks are stored row-wise
    auto &blocks()
    {
        return m_subblocks;
    }

    /// @brief Get sub-block of this one. Blocks are stored row-wise
    const auto &block(std::size_t index) const
    {
        return m_subblocks[index];
    }

    /// @brief Get sub-block of this one. Blocks are stored row-wise
    auto &block(std::size_t index)
    {
        return m_subblocks[index];
    }

private:
    value_type *m_pixels = nullptr;
    uint32_t m_width = 0;
    uint32_t m_height = 0;
    uint32_t m_x = 0;
    uint32_t m_y = 0;
    uint32_t m_blockIndex = 0;
    std::array<uint32_t, Dim * Dim> m_indices;
    std::array<BlockView<value_type, Dim / 2, MinDim>, 4> m_subblocks;
};

template <typename T>
class BlockView<T, 4, 4>
{
public:
    using value_type = T;
    static constexpr std::size_t Dim = 4;
    static constexpr std::size_t MinDim = 4;

    using Iterator = ViewIterator<value_type>;
    using ConstIterator = ViewIterator<const value_type>;

    BlockView() = default;

    BlockView(value_type *pixels, uint32_t width, uint32_t height, uint32_t x, uint32_t y)
        : m_pixels(pixels), m_width(width), m_height(height), m_x(x), m_y(y), m_blockIndex(m_y / Dim * m_width / Dim + m_x / Dim)
    {
        auto offset = m_y * m_width + m_x;
        auto index = 0;
        for (std::size_t j = 0; j < Dim; ++j)
        {
            for (std::size_t i = 0; i < Dim; ++i)
            {
                m_indices[index++] = offset + i;
            }
            offset += m_width;
        }
    }

    BlockView &operator=(const std::array<value_type, Dim * Dim> &pixels)
    {
        for (std::size_t i = 0; i < Dim * Dim; ++i)
        {
            m_pixels[m_indices[i]] = pixels[i];
        }
        return *this;
    }

    /// @brief Return block index in image. Blocks are stored row-wise.
    /// Each block level has their own block indices
    auto index() const
    {
        return m_blockIndex;
    }

    /// @brief Return x-position
    auto x() const
    {
        return m_x;
    }

    /// @brief Return y-position
    auto y() const
    {
        return m_y;
    }

    Iterator begin() noexcept
    {
        return Iterator(m_pixels, m_indices.data(), 0);
    }

    Iterator end() noexcept
    {
        return Iterator(m_pixels, m_indices.data(), m_indices.size());
    }

    ConstIterator cbegin() const noexcept
    {
        return ConstIterator(m_pixels, m_indices.data(), 0);
    }

    ConstIterator cend() const noexcept
    {
        return ConstIterator(m_pixels, m_indices.data(), m_indices.size());
    }

    auto empty() const -> bool
    {
        return m_indices.size() == 0;
    }

    auto size() const -> std::size_t
    {
        return m_indices.size();
    }

    auto operator[](std::size_t index) const -> const value_type &
    {
        return m_pixels[m_indices[index]];
    }

    auto operator[](std::size_t index) -> value_type &
    {
        return m_pixels[m_indices[index]];
    }

    /// @brief Return block pixels as deep-copy compact array
    auto pixels() const -> std::vector<value_type>
    {
        std::vector<value_type> result(Dim * Dim);
        for (std::size_t i = 0; i < Dim * Dim; ++i)
        {
            result[i] = m_pixels[m_indices[i]];
        }
        return result;
    }

    /// @brief Deep copy pixels from other block into this one
    auto copyPixelsFrom(const BlockView &other) -> void
    {
        for (std::size_t i = 0; i < Dim * Dim; ++i)
        {
            m_pixels[m_indices[i]] = other.m_pixels[other.m_indices[i]];
        }
    }

    /// @brief Deep copy pixels from other block into this one
    auto copyPixelsFrom(const std::vector<value_type> &pixels) -> void
    {
        REQUIRE(pixels.size() == Dim * Dim, std::runtime_error, "Data must have same size as block");
        for (std::size_t i = 0; i < Dim * Dim; ++i)
        {
            m_pixels[m_indices[i]] = pixels[i];
        }
    }

private:
    value_type *m_pixels = nullptr;
    uint32_t m_width = 0;
    uint32_t m_height = 0;
    uint32_t m_x = 0;
    uint32_t m_y = 0;
    uint32_t m_blockIndex = 0;
    std::array<uint32_t, Dim * Dim> m_indices;
};

/// @brief Calculate perceived pixel difference between blocks
template <typename COLOR_TYPE, std::size_t BLOCK_DIM>
static auto mse(const BlockView<COLOR_TYPE, BLOCK_DIM> &a, const BlockView<COLOR_TYPE, BLOCK_DIM> &b) -> float
{
    double dist = 0.0;
    for (auto aIt = a.cbegin(), bIt = b.cbegin(); aIt != a.cend() && bIt != b.cend(); ++aIt, ++bIt)
    {
        dist += COLOR_TYPE::mse(*aIt, *bIt);
    }
    return static_cast<float>(dist / (BLOCK_DIM * BLOCK_DIM));
}

/// @brief Calculate perceived pixel difference between blocks
template <typename COLOR_TYPE, std::size_t BLOCK_DIM>
static auto mseBelowThreshold(const BlockView<COLOR_TYPE, BLOCK_DIM> &a, const BlockView<COLOR_TYPE, BLOCK_DIM> &b, float threshold) -> std::pair<bool, float>
{
    bool belowThreshold = true;
    double dist = 0.0;
    for (auto aIt = a.cbegin(), bIt = b.cbegin(); aIt != a.cend() && bIt != b.cend(); ++aIt, ++bIt)
    {
        auto colorDist = COLOR_TYPE::mse(*aIt, *bIt);
        if (belowThreshold)
        {
            belowThreshold = colorDist < threshold;
        }
        dist += colorDist;
    }
    return {belowThreshold, static_cast<float>(dist / (BLOCK_DIM * BLOCK_DIM))};
}