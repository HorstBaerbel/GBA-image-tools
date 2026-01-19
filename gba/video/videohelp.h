#pragma once

#include "color.h"

#include <cstdint>

namespace VideoHelp
{
    /// @brief The symbols the help can display
    enum class Symbol : uint8_t
    {
        Play = 0,
        Stop = 1,
        Help = 2,
        Subtitles = 3,
        Previous = 4,
        Next = 5
    };

    /// @brief Set up Video help: Does not change display mode, but enable sprites
    /// @param spriteStartIndex Index at which the sprites start
    /// @param tileStartIndex Index at which the tiles for the sprites start
    /// @param paletteIndex 16-color palette index for sprites
    auto Setup(uint32_t spriteStartIndex = 0, uint32_t tileStartIndex = 0, uint16_t paletteIndex = 0) -> void;

    /// @brief Get the number of sprites in use by video help
    auto SpritesInUse() -> uint32_t;

    /// @brief Get the number of tiles in use by video help
    auto TilesInUse() -> uint32_t;

    /// @brief Set foreground color. Applied immediately
    auto SetColor(color16 textColor = COLOR16_WHITE) -> void;

    /// @brief Enable or disable a symbol for display
    auto SetSymbolEnabled(Symbol symbol, bool enable = true) -> void;

    /// @brief Display video help on screen / copy sprites to OAM
    auto Present() -> void;

    /// @brief Show or hide current subtitle. Call Present() to update display
    auto SetVisible(bool visible) -> void;

    /// @brief Clear video help. Call Present() to update display
    auto Clear() -> void;

    /// @brief Clean up video help: Disable sprites
    auto Cleanup() -> void;
}
