#include "subtitles.h"

#include <video.h>

#include "memory/memory.h"
#include "palette.h"
#include "print/itoa.h"
#include "sprites.h"
#include "tiles.h"

#include "./data/font_sans.h"
#include "./data/font_sans_chars.h"

namespace Subtitles
{
    constexpr const uint16_t CGA_COLORS[16]{0, 0x5000, 0x280, 0x5280, 0x0014, 0x5014, 0x0154, 0x5294, 0x294a, 0x7d4a, 0x2bea, 0x7fea, 0x295f, 0x7d5f, 0x2bff, 0x7fff};
    constexpr Color backColor = Color::Black;
    constexpr Color textColor = Color::White;
    constexpr uint32_t MaxSubtitlesChars = 64;
    EWRAM_DATA uint32_t spritesInUse = 0;
    EWRAM_DATA Sprites::Sprite2D *sprites = nullptr;

    auto setup() -> void
    {
        // disable sprites
        REG_DISPCNT &= ~OBJ_ON;
        // clear all sprites
        Sprites::clearOAM();
        // build sprite color palette #0
        Memory::memset16(Palette::Sprite, 0, 16);
        Palette::Sprite[1] = CGA_COLORS[15];
        // allocate sprites
        spritesInUse = 0;
        sprites = Memory::malloc_EWRAM<Sprites::Sprite2D>(MaxSubtitlesChars);
        Sprites::create(sprites, MaxSubtitlesChars, 0, 512, Sprites::SizeCode::Size8x8, Sprites::ColorDepth::Depth16);
        // copy data to tile map char by char
        uint32_t charPos = 0; // horizontal position in bitmap our char is in
        for (uint32_t ci = 0; ci < FONT_SANS_NR_OF_CHARS; ++ci)
        {
            auto spriteTile = Sprites::TILE_INDEX_TO_MEM<uint32_t>(512 + ci);
            const uint32_t charWidth = FONT_SANS_CHAR_WIDTH[ci] > 8 ? 8 : FONT_SANS_CHAR_WIDTH[ci];
            auto charLinePtr = &(reinterpret_cast<const uint8_t *>(FONT_SANS_DATA))[charPos];
            for (uint32_t y = 0; y < FONT_SANS_HEIGHT; ++y)
            {
                uint32_t pixelLine = 0;
                for (uint32_t x = 0; x < charWidth; ++x)
                {
                    pixelLine = (pixelLine >> 4) | ((charLinePtr[x] & 0x0F) << 28);
                }
                pixelLine >>= (8 - charWidth) * 4;
                spriteTile[y] = pixelLine;
                charLinePtr += FONT_SANS_WIDTH;
            }
            charPos += charWidth;
        }
        // enable sprites
        REG_DISPCNT |= OBJ_ON | OBJ_1D_MAP;
    }

    auto getScreenWidth(const char *s) -> uint32_t
    {
        if (s == nullptr)
        {
            return 0;
        }
        uint32_t screenWidth = 0;
        uint32_t charCount = 0;
        while (*s != '\0' && charCount < MaxSubtitlesChars)
        {
            auto charIndex = static_cast<int32_t>(*s) - 32;
            if (charIndex >= 0 && charIndex < static_cast<int32_t>(FONT_SANS_NR_OF_CHARS))
            {
                screenWidth += FONT_SANS_CHAR_WIDTH[charIndex] - 1;
                ++charCount;
            }
            ++s;
        }
        return screenWidth;
    }

    auto getStringLength(const char *s) -> uint32_t
    {
        if (s == nullptr)
        {
            return 0;
        }
        uint32_t charCount = 0;
        while (*s != '\0' && charCount < MaxSubtitlesChars)
        {
            auto charIndex = static_cast<int32_t>(*s) - 32;
            if (charIndex >= 0 && charIndex < static_cast<int32_t>(FONT_SANS_NR_OF_CHARS))
            {
                ++charCount;
            }
            ++s;
        }
        return charCount;
    }

    auto printString(const char *s, int16_t x, int16_t y, Color textColor) -> void
    {
        if (s == nullptr)
        {
            return;
        }
        int16_t charX = x;
        uint32_t charCount = 0;
        while (*s != '\0' && charCount < MaxSubtitlesChars)
        {
            auto charIndex = static_cast<int32_t>(*s) - 32;
            if (charIndex >= 0 && charIndex < static_cast<int32_t>(FONT_SANS_NR_OF_CHARS))
            {
                auto &sprite = sprites[spritesInUse + charCount];
                sprite.tileIndex = 512 + charIndex;
                sprite.x = charX;
                sprite.y = y;
                sprite.visible = true;
                sprite.priority = Sprites::Priority::Prio0;
                charX += FONT_SANS_CHAR_WIDTH[charIndex] - 1;
                ++charCount;
            }
            ++s;
        }
        spritesInUse += charCount;
        setColor(textColor);
    }

    auto setColor(Color text) -> void
    {
        Palette::Sprite[1] = CGA_COLORS[static_cast<uint8_t>(text)];
    }

    auto display() -> void
    {
        for (uint32_t i = spritesInUse; i < MaxSubtitlesChars; ++i)
        {
            auto &sprite = sprites[i];
            sprite.visible = false;
            sprite.priority = Sprites::Priority::Prio3;
        }
        Sprites::copyToOAM(sprites, 0, MaxSubtitlesChars);
    }

    auto clear() -> void
    {
        for (uint32_t i = 0; i < spritesInUse; ++i)
        {
            auto &sprite = sprites[i];
            sprite.visible = false;
            sprite.priority = Sprites::Priority::Prio3;
        }
        spritesInUse = 0;
    }

    auto cleanup() -> void
    {
        REG_DISPCNT &= ~OBJ_ON;
        spritesInUse = 0;
        Sprites::clearOAM();
        Memory::free(sprites);
    }
}