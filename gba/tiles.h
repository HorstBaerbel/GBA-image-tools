#pragma once

/// @brief Functions for tile-base backgrounds in modes 0/1/2
namespace Tiles
{

    /// @brief s-tile 8x8@4bpp: 32 bytes, 8 words
    struct Tile16
    {
        uint32_t data[8];
    };

    /// @brief d-tile: 8x8@8bpp: 64 bytes, 16 words
    struct Tile256
    {
        uint32_t data[16];
    };

    /// @brief Start of tile memory (== start VRAM)
    constexpr uint32_t TileMem = 0x06000000;

    /// @brief Tile memory interpreted as 16 color tiles
    auto const TileMem16{reinterpret_cast<Tile16 *>(TileMem)};

    /// @brief Tile memory interpreted as 256 color tiles
    auto const TileMem256{reinterpret_cast<Tile256 *>(TileMem)};

    /// @brief This is where the tiles aka the bitmap / pixel data for the tiles starts
    enum class TileBase : uint16_t
    {
        Base_0000 = (0 << 2),
        Base_4000 = (1 << 2),
        Base_8000 = (2 << 2),
        Base_C000 = (3 << 2)
    };

    /// @brief Convert a tile base value to a VRAM address
    constexpr uint16_t *TILE_BASE_TO_MEM(TileBase b)
    {
        return reinterpret_cast<uint16_t *>(TileMem + ((uint32_t(b)) << 12));
    }

    /// @brief This is where the screen / tile layout aka the map data for the tiles starts
    enum class ScreenBase : uint16_t
    {
        Base_0000 = (0 << 8),
        Base_0800 = (1 << 8),
        Base_1000 = (2 << 8),
        Base_1800 = (3 << 8),
        Base_2000 = (4 << 8),
        Base_2800 = (5 << 8),
        Base_3000 = (6 << 8),
        Base_3800 = (7 << 8),
        Base_4000 = (8 << 8),
        Base_4800 = (9 << 8),
        Base_5000 = (10 << 8),
        Base_5800 = (11 << 8),
        Base_6000 = (12 << 8),
        Base_6800 = (13 << 8),
        Base_7000 = (14 << 8),
        Base_7800 = (15 << 8),
        Base_8000 = (16 << 8),
        Base_8800 = (17 << 8),
        Base_9000 = (18 << 8),
        Base_9800 = (19 << 8),
        Base_A000 = (20 << 8),
        Base_A800 = (21 << 8),
        Base_B000 = (22 << 8),
        Base_B800 = (23 << 8),
        Base_C000 = (24 << 8),
        Base_C800 = (25 << 8),
        Base_D000 = (26 << 8),
        Base_D800 = (27 << 8),
        Base_E000 = (28 << 8),
        Base_E800 = (29 << 8),
        Base_F000 = (30 << 8),
        Base_F800 = (31 << 8)
    };

    /// @brief Convert a screen base value to a VRAM address
    constexpr uint16_t *SCREEN_BASE_TO_MEM(ScreenBase b)
    {
        return reinterpret_cast<uint16_t *>(TileMem + ((uint32_t(b)) << 3));
    }

    /// @brief Tiled background screen size
    enum class ScreenSize : uint16_t
    {
        Size_0 = (0 << 14), // Text mode: 256x256, Rotation/Scaling mode: 128x128
        Size_1 = (1 << 14), // Text mode: 512x256, Rotation/Scaling mode: 256x256
        Size_2 = (2 << 14), // Text mode: 256x512, Rotation/Scaling mode: 512x512
        Size_3 = (3 << 14)  // Text mode: 512x512, Rotation/Scaling mode: 1024x1024
    };

    /// @brief Build tile background config
    constexpr uint16_t background(TileBase tileBase, ScreenBase screenBase, ScreenSize screenSize = ScreenSize::Size_0, uint16_t paletteColors = 16, uint16_t priority = 0, bool mosaic = false)
    {
        return static_cast<uint16_t>(tileBase) | static_cast<uint16_t>(screenBase) | static_cast<uint16_t>(screenSize) | (paletteColors <= 16 ? 0 : (1 << 7)) | (priority & 3);
    }

} // namespace Tiles
