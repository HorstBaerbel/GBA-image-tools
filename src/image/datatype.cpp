#include "datatype.h"

namespace Image
{

    auto DataType::setBitmap(bool value) -> void
    {
        flags = value ? flags | static_cast<uint32_t>(Flags::Bitmap) : flags & ~static_cast<uint32_t>(Flags::Bitmap);
    }

    auto DataType::isBitmap() const -> bool
    {
        return (flags & static_cast<uint32_t>(Flags::Bitmap)) != 0;
    }

    auto DataType::setSprites(bool value) -> void
    {
        flags = value ? flags | static_cast<uint32_t>(Flags::Sprites) : flags & ~static_cast<uint32_t>(Flags::Sprites);
    }

    auto DataType::isSprites() const -> bool
    {
        return (flags & static_cast<uint32_t>(Flags::Sprites)) != 0;
    }

    auto DataType::setTiles(bool value) -> void
    {
        flags = value ? flags | static_cast<uint32_t>(Flags::Tiles) : flags & ~static_cast<uint32_t>(Flags::Tiles);
    }

    auto DataType::isTiles() const -> bool
    {
        return (flags & static_cast<uint32_t>(Flags::Tiles)) != 0;
    }

    auto DataType::setCompressed(bool value) -> void
    {
        flags = value ? flags | static_cast<uint32_t>(Flags::Compressed) : flags & ~static_cast<uint32_t>(Flags::Compressed);
    }

    auto DataType::isCompressed() const -> bool
    {
        return (flags & static_cast<uint32_t>(Flags::Compressed)) != 0;
    }

};
