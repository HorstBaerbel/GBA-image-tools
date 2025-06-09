#pragma once

#include "exception.h"

#include <cstdint>

namespace Image
{

    /// @brief Image data type flags
    class DataType
    {
    public:
        enum class Flags : uint32_t
        {
            Bitmap = 1,
            Sprites = 2,
            Tiles = 4,
            Compressed = 8
        };

        constexpr DataType() = default;
        constexpr DataType(const Flags &f)
            : flags(static_cast<uint32_t>(f))
        {
        }

        auto setBitmap(bool value = true) -> void;
        auto isBitmap() const -> bool;

        auto setSprites(bool value = true) -> void;
        auto isSprites() const -> bool;

        auto setTiles(bool value = true) -> void;
        auto isTiles() const -> bool;

        auto setCompressed(bool value = true) -> void;
        auto isCompressed() const -> bool;

    private:
        uint32_t flags = 0;
    };
}