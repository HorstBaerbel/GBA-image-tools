#include "tui.h"

#include <gba_video.h>

#include "itoa.h"
#include "memory.h"
#include "tiles.h"

namespace TUI
{
    static const uint16_t CGA_COLORS[16]{0, 0x5000, 0x280, 0x5280, 0x0014, 0x5014, 0x0154, 0x5294, 0x294a, 0x7d4a, 0x2bea, 0x7fea, 0x295f, 0x7d5f, 0x2bff, 0x7fff};
    static Color backColor = Color::Black;
    static Color textColor = Color::White;
    static char PrintBuffer[64] = {0};

    void setup()
    {
        // set graphics to mode 0 and enable background 0 and 1
        REG_DISPCNT = MODE_0 | BG0_ON | BG1_ON;
        // copy data to tile map
        Memory::memcpy32(Tiles::TILE_BASE_TO_MEM(Tiles::TileBase::Base_0000), FONT_8X8_DATA, FONT_8X8_DATA_SIZE);
        // set up background 0 (text background) and 1 (text foreground) screen and tile map starts and set screen size to 256x256
        REG_BG0CNT = Tiles::background(Tiles::TileBase::Base_0000, Tiles::ScreenBase::Base_1000, Tiles::ScreenSize::Size_0, 16, 1);
        REG_BG1CNT = Tiles::background(Tiles::TileBase::Base_0000, Tiles::ScreenBase::Base_2000, Tiles::ScreenSize::Size_0, 16, 0);
        // build CGA color palette, yesss!!!
        for (uint32_t i = 0; i < 16; i++)
        {
            BG_PALETTE[i * 16] = 0;
            BG_PALETTE[i * 16 + 1] = CGA_COLORS[i];
        }
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

    uint16_t printInt(int32_t value, uint32_t base, uint16_t x, uint16_t y, Color backColor, Color textColor)
    {
        itoa(value, PrintBuffer, base);
        return printString(PrintBuffer, x, y, backColor, textColor);
    }

    uint16_t printFloat(int32_t value, uint16_t x, uint16_t y, Color backColor, Color textColor)
    {
        fptoa(value, PrintBuffer, 8, 2);
        return printString(PrintBuffer, x, y, backColor, textColor);
    }

    void setColor(Color backColor, Color textColor)
    {
    }

    void printf(uint16_t x, uint16_t y, const char *fmt...)
    {
        auto currentX = x;
        va_list args;
        va_start(args, fmt);
        bool expectType = false;
        while (*fmt != '\0')
        {
            if (expectType)
            {
                expectType = false;
                if (*fmt == 'd')
                {
                    auto i = va_arg(args, int32_t);
                    currentX += printInt(i, 10, currentX, y, backColor, textColor);
                }
                else if (*fmt == 'x')
                {
                    auto i = va_arg(args, int32_t);
                    currentX += printInt(i, 16, currentX, y, backColor, textColor);
                }
                else if (*fmt == 'c')
                {
                    // note automatic conversion to integral type
                    auto c = va_arg(args, int32_t);
                    printChar(c, currentX++, y, backColor, textColor);
                }
                else if (*fmt == 'f')
                {
                    auto f = va_arg(args, int32_t);
                    currentX += printFloat(f, currentX, y, backColor, textColor);
                }
                else
                {
                    printChar(*fmt, currentX++, y, backColor, textColor);
                }
            }
            else if (*fmt == '%')
            {
                expectType = true;
            }
            else
            {
                printChar(*fmt, currentX++, y, backColor, textColor);
            }
            ++fmt;
        }
        va_end(args);
    }

}