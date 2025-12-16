#pragma once

#include <base.h>

#include <cstdint>

namespace Subtitles
{
    /// @brief CGA colors. See: https://en.wikipedia.org/wiki/Color_Graphics_Adapter
    enum class Color : uint16_t
    {
        Black = 0,
        Blue = 1,
        Green = 2,
        Cyan = 3,
        Red = 4,
        Magenta = 5,
        Brown = 6,
        LightGray = 7,
        DarkGray = 8,
        LightBlue = 9,
        LightGreen = 10,
        LightCyan = 11,
        LightRed = 12,
        LightMagenta = 13,
        Yellow = 14,
        White = 15
    };

    /// @brief Set up Subtitles mode: Don't change display mode, but enable sprites
    void setup();

    /// @brief Set background and foreground colors
    void setColor(Color backColor = Color::Black, Color textColor = Color::White);

    void printChar(char c, uint16_t x, uint16_t y, Color backColor = Color::Black, Color textColor = Color::White);
    uint16_t printChars(char c, uint16_t n, uint16_t x, uint16_t y, Color backColor = Color::Black, Color textColor = Color::White);
    uint16_t printString(const char *s, uint16_t x, uint16_t y, Color backColor = Color::Black, Color textColor = Color::White);
}
