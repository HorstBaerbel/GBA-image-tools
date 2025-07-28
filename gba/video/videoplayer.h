#pragma once

#include "vid2hio.h"

#include <cstdint>

namespace Media
{

    /// @brief Initialize player for media stream. Call before using any of the other functions
    /// @param mediaSrc Media source data
    /// @param videoScratchPad Intermediate memory for decoding. Can be nullptr if you only have one compression stage. Must be aligned to 4 bytes!
    /// @param videoScratchPadSize Size of intermediate memory for decoding. Must be a multiple of 4 bytes!
    /// @param audioScratchPad Memory for storing audio sample data. Must be in IWRAM. Must be aligned to 4 bytes!
    /// @param audioScratchPadSize Size of memory for storing audio sample data. Must be a multiple of 4 bytes!
    /// @note Will use Timer 0, 1, 2, IRQ 1, 2 and DMA 1 (mono) or DMA 1 + 2 (stereo). Also modifies sound registers, especially REG_SOUNDCNT_X
    auto Init(const uint32_t *mediaSrc, uint32_t *videoScratchPad, uint32_t videoScratchPadSize, uint32_t *audioScratchPad, uint32_t audioScratchPadSize) -> void;

    /// @brief Set color that screen and scratchpad will be set to when starting playback
    /// @param color
    auto SetClearColor(uint16_t color) -> void;

    /// @brief Get media information
    auto GetInfo() -> const IO::Vid2h::Info &;

    /// @brief Start playing media
    auto Play() -> void;

    /// @brief Stop playing media
    auto Stop() -> void;

    /// @brief Check if there are more frames in the media data
    /// @return True if the media data has more frames, false if this is the last frame
    auto HasMoreFrames() -> bool;

    /// @brief Decode the next frame(s) from media data
    /// The player will only decode new frames when it needs to due to the frame rate of the video.
    auto DecodeAndPlay() -> void;
}
