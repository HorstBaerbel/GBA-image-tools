#pragma once

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

/// @brief Struct describing an W*H block of colors that references part of an image.
/// It does not hold the color data itself, but merely references it.
template <typename T, std::size_t W, std::size_t H, size_t MIN_W = 4>
class BlockView
{
};

template <typename T, std::size_t W, std::size_t H>
class BlockView<T, W, H>
{
public:
    using value_type = T;
    static constexpr std::size_t Width = W;
    static constexpr std::size_t Height = H;
    static constexpr std::size_t MinWidth = 4;

    using Iterator = ViewIterator<value_type>;
    using ConstIterator = ViewIterator<const value_type>;

    BlockView() = default;

    BlockView(value_type *colors, uint32_t width, uint32_t height, uint32_t x, uint32_t y)
        : m_colors(colors), m_width(width), m_height(height), m_x(x), m_y(y), m_blockIndex(m_y / Height * m_width / Width + m_x / Width)
    {
        auto offset = m_y * m_width + m_x;
        auto index = 0;
        for (std::size_t j = 0; j < Height; ++j)
        {
            for (std::size_t i = 0; i < Width; ++i)
            {
                m_indices[index++] = offset + i;
            }
            offset += m_width;
        }
        if constexpr (Width > MinWidth)
        {
            m_subblocks[0] = BlockView<value_type, Width / 2, Height, MinWidth>(m_colors, m_width, m_height, x, y);
            m_subblocks[1] = BlockView<value_type, Width / 2, Height, MinWidth>(m_colors, m_width, m_height, x + Width / 2, y);
        }
    }

    BlockView &operator=(const std::array<value_type, Width * Height> &colors)
    {
        for (std::size_t i = 0; i < Width * Height; ++i)
        {
            m_colors[m_indices[i]] = colors[i];
        }
        return *this;
    }

    /// @brief Return block index in image. Blocks are stored row-wise.
    /// Each block level has their own block indices
    auto index() const -> uint32_t
    {
        return m_blockIndex;
    }

    Iterator begin() noexcept
    {
        return Iterator(m_colors, m_indices.data(), 0);
    }

    Iterator end() noexcept
    {
        return Iterator(m_colors, m_indices.data(), m_indices.size());
    }

    ConstIterator cbegin() const noexcept
    {
        return ConstIterator(m_colors, m_indices.data(), 0);
    }

    ConstIterator cend() const noexcept
    {
        return ConstIterator(m_colors, m_indices.data(), m_indices.size());
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
        return m_colors[m_indices[index]];
    }

    auto operator[](std::size_t index) -> value_type &
    {
        return m_colors[m_indices[index]];
    }

    /// @brief Return block colors as deep-copy compact array
    auto colors() const
    {
        std::array<value_type, Width * Height> result;
        for (std::size_t i = 0; i < Width * Height; ++i)
        {
            result[i] = m_colors[m_indices[i]];
        }
        return result;
    }

    /// @brief Deep copy colors from other block into this one
    auto copyColorsFrom(const BlockView &other) -> void
    {
        for (std::size_t i = 0; i < Width * Height; ++i)
        {
            m_colors[m_indices[i]] = other.m_colors[other.m_indices[i]];
        }
    }

    /// @brief Deep copy colors from other block into this one
    auto copyColorsFrom(const std::array<value_type, Width * Height> &colors) -> void
    {
        for (std::size_t i = 0; i < Width * Height; ++i)
        {
            m_colors[m_indices[i]] = colors[i];
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
    value_type *m_colors = nullptr;
    uint32_t m_width = 0;
    uint32_t m_height = 0;
    uint32_t m_x = 0;
    uint32_t m_y = 0;
    uint32_t m_blockIndex = 0;
    std::array<uint32_t, Width * Height> m_indices;
    std::array<BlockView<value_type, Width / 2, Height, MinWidth>, 2> m_subblocks;
};

template <typename T>
class BlockView<T, 4, 4, 4>
{
public:
    using value_type = T;
    static constexpr std::size_t Width = 4;
    static constexpr std::size_t Height = 4;
    static constexpr std::size_t MinWidth = 4;

    using Iterator = ViewIterator<value_type>;
    using ConstIterator = ViewIterator<const value_type>;

    BlockView() = default;

    BlockView(value_type *colors, uint32_t width, uint32_t height, uint32_t x, uint32_t y)
        : m_colors(colors), m_width(width), m_height(height), m_x(x), m_y(y), m_blockIndex(m_y / Height * m_width / Width + m_x / Width)
    {
        auto offset = m_y * m_width + m_x;
        auto index = 0;
        for (std::size_t j = 0; j < Height; ++j)
        {
            for (std::size_t i = 0; i < Width; ++i)
            {
                m_indices[index++] = offset + i;
            }
            offset += m_width;
        }
    }

    BlockView &operator=(const std::array<value_type, Width * Height> &colors)
    {
        for (std::size_t i = 0; i < Width * Height; ++i)
        {
            m_colors[m_indices[i]] = colors[i];
        }
        return *this;
    }

    /// @brief Return block index in image. Blocks are stored row-wise.
    /// Each block level has their own block indices
    auto index() const
    {
        return m_blockIndex;
    }

    Iterator begin() noexcept
    {
        return Iterator(m_colors, m_indices.data(), 0);
    }

    Iterator end() noexcept
    {
        return Iterator(m_colors, m_indices.data(), m_indices.size());
    }

    ConstIterator cbegin() const noexcept
    {
        return ConstIterator(m_colors, m_indices.data(), 0);
    }

    ConstIterator cend() const noexcept
    {
        return ConstIterator(m_colors, m_indices.data(), m_indices.size());
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
        return m_colors[m_indices[index]];
    }

    auto operator[](std::size_t index) -> value_type &
    {
        return m_colors[m_indices[index]];
    }

    /// @brief Return block colors as deep-copy compact array
    auto colors() const
    {
        std::array<value_type, Width * Height> result;
        for (std::size_t i = 0; i < Width * Height; ++i)
        {
            result[i] = m_colors[m_indices[i]];
        }
        return result;
    }

    /// @brief Deep copy colors from other block into this one
    auto copyColorsFrom(const BlockView &other) -> void
    {
        for (std::size_t i = 0; i < Width * Height; ++i)
        {
            m_colors[m_indices[i]] = other.m_colors[other.m_indices[i]];
        }
    }

    /// @brief Deep copy colors from other block into this one
    auto copyColorsFrom(const std::array<value_type, Width * Height> &colors) -> void
    {
        for (std::size_t i = 0; i < Width * Height; ++i)
        {
            m_colors[m_indices[i]] = colors[i];
        }
    }

private:
    value_type *m_colors = nullptr;
    uint32_t m_width = 0;
    uint32_t m_height = 0;
    uint32_t m_x = 0;
    uint32_t m_y = 0;
    uint32_t m_blockIndex = 0;
    std::array<uint32_t, Width * Height> m_indices;
};
