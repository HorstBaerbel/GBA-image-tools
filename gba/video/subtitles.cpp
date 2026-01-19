#include "subtitles.h"

#include "memory/memory.h"
#include "palette.h"
#include "print/itoa.h"
#include "sprites.h"
#include "tiles.h"

#include "./data/font_subtitles.h"
#include "./data/font_subtitles_chars.h"

namespace Subtitles
{
    constexpr uint32_t MaxSubtitlesChars = 64;
    EWRAM_DATA Sprites::Sprite2D *sprites = nullptr;
    EWRAM_DATA uint16_t spritesInUse = 0;
    EWRAM_DATA uint16_t spritePaletteIndex = 0;
    EWRAM_DATA uint16_t spriteTileIndex = 0;
    EWRAM_DATA bool spritesVisible = true;

    auto Setup(uint32_t spriteStartIndex, uint32_t tileStartIndex, uint16_t paletteIndex) -> void
    {
        // disable sprites
        REG_DISPCNT &= ~OBJ_ON;
        // clear all sprites
        Sprites::clearOAM();
        // build sprite color palette
        spritePaletteIndex = paletteIndex;
        Memory::memset16(Palette::Sprite16[spritePaletteIndex], 0, 16);
        Palette::Sprite16[spritePaletteIndex][1] = 0x7fff;
        // allocate sprites
        spritesInUse = 0;
        sprites = Memory::malloc_EWRAM<Sprites::Sprite2D>(MaxSubtitlesChars);
        spriteTileIndex = tileStartIndex;
        Sprites::create(sprites, MaxSubtitlesChars, spriteStartIndex, spriteTileIndex, Sprites::SizeCode::Size8x8, Sprites::ColorDepth::Depth16, spritePaletteIndex);
        // copy data to tile map
        auto spriteTile = Sprites::TILE_INDEX_TO_MEM<uint32_t>(spriteTileIndex);
        Memory::memcpy32(spriteTile, FONT_SUBTITLES_DATA, FONT_SUBTITLES_DATA_SIZE);
        // enable sprites
        REG_DISPCNT |= OBJ_ON | OBJ_1D_MAP;
    }

    auto SpritesInUse() -> uint32_t
    {
        return sprites != nullptr ? spritesInUse : 0;
    }

    auto TilesInUse() -> uint32_t
    {
        if (sprites != nullptr && spritesInUse > 0)
        {
            const auto tileStart = sprites[0].tileIndex;
            const auto &sprite = sprites[spritesInUse - 1];
            return sprite.tileIndex + Tiles::TileCountForSizeCode[static_cast<uint32_t>(sprite.size)] - tileStart;
        }
        return 0;
    }

    auto GetScreenWidth(const char *string, const char *end) -> uint32_t
    {
        if (string == nullptr)
        {
            return 0;
        }
        uint32_t screenWidth = 0;
        uint32_t charCount = 0;
        while (*string != '\0' && string < end)
        {
            auto charIndex = static_cast<int32_t>(*string) - 32;
            if (charIndex >= 0 && charIndex < static_cast<int32_t>(FONT_SUBTITLES_NR_OF_CHARS))
            {
                screenWidth += FONT_SUBTITLES_CHAR_WIDTH[charIndex] + 1;
                ++charCount;
            }
            ++string;
        }
        return screenWidth - 1;
    }

    auto GetNrOfLines(const char *string) -> uint32_t
    {
        if (string == nullptr)
        {
            return 0;
        }
        uint32_t nrOfLines = 1;
        while (*string != '\0')
        {
            nrOfLines += *string == '\n' ? 1 : 0;
            ++string;
        }
        return nrOfLines;
    }

    auto GetStringLength(const char *string, const char *end) -> uint32_t
    {
        if (string == nullptr)
        {
            return 0;
        }
        uint32_t charCount = 0;
        while (*string != '\0' && string < end)
        {
            auto charIndex = static_cast<int32_t>(*string) - 32;
            if (charIndex >= 0 && charIndex < static_cast<int32_t>(FONT_SUBTITLES_NR_OF_CHARS))
            {
                ++charCount;
            }
            ++string;
        }
        return charCount;
    }

    auto PrintString(const char *string, const char *end, int16_t x, int16_t y, color16 textColor) -> void
    {
        if (string == nullptr)
        {
            return;
        }
        int16_t charX = x;
        while (*string != '\0' && string < end && spritesInUse < MaxSubtitlesChars)
        {
            auto charIndex = static_cast<int32_t>(*string) - 32;
            if (charIndex >= 0 && charIndex < static_cast<int32_t>(FONT_SUBTITLES_NR_OF_CHARS))
            {
                auto &sprite = sprites[spritesInUse];
                sprite.tileIndex = spriteTileIndex + charIndex;
                sprite.x = charX;
                sprite.y = y;
                sprite.priority = Sprites::Priority::Prio0;
                charX += FONT_SUBTITLES_CHAR_WIDTH[charIndex] + 1;
                ++spritesInUse;
            }
            ++string;
        }
        SetColor(textColor);
    }

    auto SetColor(color16 textColor) -> void
    {
        Palette::Sprite16[spritePaletteIndex][1] = textColor;
    }

    auto Present() -> void
    {
        // enable used sprites depending on visibility
        for (uint32_t i = 0; i < spritesInUse; ++i)
        {
            auto &sprite = sprites[i];
            sprite.visible = spritesVisible;
            sprite.priority = spritesVisible ? Sprites::Priority::Prio0 : Sprites::Priority::Prio3;
        }
        // hide unused sprites
        for (uint32_t i = spritesInUse; i < MaxSubtitlesChars; ++i)
        {
            auto &sprite = sprites[i];
            sprite.visible = false;
            sprite.priority = Sprites::Priority::Prio3;
        }
        Sprites::copyToOAM(sprites, 0, MaxSubtitlesChars);
    }

    auto SetVisible(bool visible) -> void
    {
        spritesVisible = visible;
    }

    auto Clear() -> void
    {
        spritesInUse = 0;
    }

    auto Cleanup() -> void
    {
        REG_DISPCNT &= ~OBJ_ON;
        spritesInUse = 0;
        spritePaletteIndex = 0;
        spriteTileIndex = 0;
        spritesVisible = true;
        Sprites::clearOAM();
        Memory::free(sprites);
    }
}