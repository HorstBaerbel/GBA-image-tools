#pragma once

#include <gba_base.h>

#include <cstdint>

#include "./data/font_8x8.h"

namespace TUI
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

    static constexpr uint32_t Width = 32;
    static constexpr uint32_t Height = 20;

    /// @brief Set up TUI mode: Mode0, BG0 + BG1 on
    /// Provides you with a noice 32x20 screen CGA color palette console text UI \o/
    void setup();

    /// @brief Fill whole background with color
    void fillBackground(Color color);

    /// @brief Fill background rect with color
    void fillBackgroundRect(uint16_t x, uint16_t y, uint16_t width, uint16_t height, Color color = Color::Black);

    void printChar(char c, uint16_t x, uint16_t y, Color backColor = Color::Black, Color textColor = Color::White) IWRAM_CODE;
    uint16_t printChars(char c, uint16_t n, uint16_t x, uint16_t y, Color backColor = Color::Black, Color textColor = Color::White) IWRAM_CODE;
    uint16_t printString(const char *s, uint16_t x, uint16_t y, Color backColor = Color::Black, Color textColor = Color::White) IWRAM_CODE;
    uint16_t printInt(int32_t value, uint32_t base, uint16_t x, uint16_t y, Color backColor, Color textColor) IWRAM_CODE;
    uint16_t printFloat(int32_t value, uint16_t x, uint16_t y, Color backColor, Color textColor) IWRAM_CODE;

}
