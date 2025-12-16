#include "subtitles.h"

#include <video.h>

#include "itoa.h"
#include "memory/memory.h"
#include "tiles.h"

#include "./data/font_sans.h"

namespace Subtitles
{
    static const uint16_t CGA_COLORS[16]{0, 0x5000, 0x280, 0x5280, 0x0014, 0x5014, 0x0154, 0x5294, 0x294a, 0x7d4a, 0x2bea, 0x7fea, 0x295f, 0x7d5f, 0x2bff, 0x7fff};
    static Color backColor = Color::Black;
    static Color textColor = Color::White;
    static char PrintBuffer[64] = {0};

    void setup()
    {
        // clear all sprites
        SPR_VRAM
        // build sprite color palette #0
        for (uint32_t i = 0; i < 16; i++)
        {
            SPRITE_PALETTE[i] = 0;
        }
        SPRITE_PALETTE[1] = CGA_COLORS[15];
        // enable sprites
        REG_DISPCNT |= OBJ_ON;
        // copy data to tile map
        Memory::memcpy32(Tiles::TILE_BASE_TO_MEM(Tiles::TileBase::Base_0000), FONT_SANS_DATA, FONT_SANS_DATA_SIZE);
    }

    void fillBackground(Color color)
    {
        const uint32_t value = 0x005F005F | (static_cast<uint32_t>(color) << 28) | (static_cast<uint32_t>(color) << 12);
        Memory::memset32(Tiles::SCREEN_BASE_TO_MEM(Tiles::ScreenBase::Base_1000), value, (Width * Height) >> 1);
    }

    void fillBackgroundRect(uint16_t x, uint16_t y, uint16_t w, uint16_t h, Color color)
    {
        const uint16_t value = 0x005F | (static_cast<uint32_t>(color) << 12);
        auto background = Tiles::SCREEN_BASE_TO_MEM(Tiles::ScreenBase::Base_1000) + Width * y + x;
        for (uint32_t yi = 0; yi < h; yi++)
        {
            Memory::memset16(background, value, w);
            background += Width;
        }
    }

    void printChar(char c, uint16_t x, uint16_t y, Color backColor, Color textColor)
    {
        uint16_t tileNo = static_cast<uint16_t>(c) - 32;
        uint32_t tileIndex = ((y & 31) * 32) + (x & 31);
        auto background = Tiles::SCREEN_BASE_TO_MEM(Tiles::ScreenBase::Base_1000);
        background[tileIndex] = 0x5F | ((static_cast<uint16_t>(backColor) & 15) << 12);
        auto text = Tiles::SCREEN_BASE_TO_MEM(Tiles::ScreenBase::Base_2000);
        text[tileIndex] = tileNo | ((static_cast<uint16_t>(textColor) & 15) << 12);
    }

    uint16_t printChars(char c, uint16_t n, uint16_t x, uint16_t y, Color backColor, Color textColor)
    {
        uint16_t nrOfCharsPrinted = 0;
        for (uint32_t i = 0; i < n; i++)
        {
            printChar(c, x++, y, backColor, textColor);
            nrOfCharsPrinted++;
        }
        return nrOfCharsPrinted;
    }

    uint16_t printString(const char *s, uint16_t x, uint16_t y, Color backColor, Color textColor)
    {
        uint16_t nrOfCharsPrinted = 0;
        if (s != nullptr)
        {
            while (*s != '\0')
            {
                printChar(*s, x++, y, backColor, textColor);
                s++;
                nrOfCharsPrinted++;
            }
        }
        return nrOfCharsPrinted;
    }

    void setColor(Color back, Color text)
    {
        backColor = back;
        textColor = text;
    }
}