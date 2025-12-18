#pragma once

#include <cstdint>

namespace Subtitles
{
    /// @brief CGA colors. See: https://en.wikipedia.org/wiki/Color_Graphics_Adapter
    enum class Color : uint8_t
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
    auto setup() -> void;

    /// @brief Get width of text on screen in pixels
    auto getScreenWidth(const char *s) -> uint32_t;

    /// @brief Get length of text on screen in characters
    auto getStringLength(const char *s) -> uint32_t;

    /// @brief Print string to screen using sprites
    /// Call clear() to clear all subtitles / sprites
    auto printString(const char *s, int16_t x, int16_t y, Color textColor = Color::White) -> void;

    /// @brief Set foreground color
    auto setColor(Color textColor = Color::White) -> void;

    /// @Display all subtitles on screen / copy sprites to OAM
    auto display() -> void;

    /// @brief Clear all subtitles by hiding all sprites
    auto clear() -> void;

    /// @brief Clean up Subtitles mode: Disable sprites
    auto cleanup() -> void;
}
