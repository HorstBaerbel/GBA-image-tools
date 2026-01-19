#pragma once

#include "color.h"

#include <cstdint>

namespace Subtitles
{
    constexpr unsigned FontHeight = 8;

    /// @brief Subtitles data for one subtitle entry
    struct Frame
    {
        int32_t startTimeS = 0;     // Subtitles start time in s
        int32_t endTimeS = 0;       // Subtitles end time in s
        const char *text = nullptr; // Subtitle string
    };

    /// @brief Set up Subtitles: Does not change display mode, but enable sprites
    /// @param spriteStartIndex Index at which the sprites start
    /// @param tileStartIndex Index at which the tiles for the sprites start
    /// @param paletteIndex 16-color palette index for sprites
    auto Setup(uint32_t spriteStartIndex = 0, uint32_t tileStartIndex = 0, uint16_t paletteIndex = 0) -> void;

    /// @brief Get the number of sprites in use by subtitles
    auto SpritesInUse() -> uint32_t;

    /// @brief Get the number of tiles in use by subtitles
    auto TilesInUse() -> uint32_t;

    /// @brief Get width of text on screen in pixels
    auto GetScreenWidth(const char *string, const char *end = nullptr) -> uint32_t;

    /// @brief Get number of lines of text in string
    auto GetNrOfLines(const char *string) -> uint32_t;

    /// @brief Get length of text on screen in characters
    auto GetStringLength(const char *string, const char *end = nullptr) -> uint32_t;

    /// @brief Print null-terminated string or sub-string to screen using sprites
    /// Call Present() to update display. Call clear() to clear all subtitles / sprites
    auto PrintString(const char *string, const char *end, int16_t x, int16_t y, color16 textColor = COLOR16_WHITE) -> void;

    /// @brief Set foreground color. Applied immediately
    auto SetColor(color16 textColor = COLOR16_WHITE) -> void;

    /// @brief Display all subtitles on screen / copy sprites to OAM
    auto Present() -> void;

    /// @brief Show or hide current subtitle. Call Present() to update display
    auto SetVisible(bool visible) -> void;

    /// @brief Clear all subtitles. Call Present() to update display
    auto Clear() -> void;

    /// @brief Clean up Subtitles mode: Disable sprites
    auto Cleanup() -> void;
}
