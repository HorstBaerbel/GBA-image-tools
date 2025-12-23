#pragma once

#include "vid2hio.h"

#include <cstdint>

namespace Media
{

    /// @brief Set up VRAM info for video playback. Call before Play()back!
    /// @param vramLineStride Stride of one line of VRAM pixels, e.g. 480 for mode 3
    /// @param vramPixelStride Stride of one VRAM pixel, e.g. 2 for mode 3
    auto SetDisplayInfo(const uint32_t vramLineStride8, const uint32_t vramPixelStride) -> void;

    /// @brief Set color that screen and scratchpad will be set to when starting playback. Call before Play()back!
    /// @param color
    auto SetClearColor(uint16_t color) -> void;

    /// @brief Set draw position of video. Call before Play()back!
    /// @param x Horizontal position
    /// @param y Vertical position
    /// @note Will cause corruption when setting this during playback
    auto SetPosition(uint16_t x, uint16_t y) -> void;

    /// @brief Start playing media
    /// @param media Media source data
    /// @param mediaSize Media source size in bytes
    /// @note Will allocate EWRAM and IWRAM as needed for video and audio buffers.
    /// Will use Timer 0, 1, 2, IRQ 1, 2 and DMA 1 (mono) or DMA 1 + 2 (stereo).
    /// Also modifies sound registers, especially REG_SOUNDCNT_X
    auto Play(const uint32_t *media, const uint32_t mediaSize) -> void;

    /// @brief Stop playing media
    auto Stop() -> void;

    /// @brief Check if there are more frames in the media data
    /// @return True if the media data has more frames, false if this is the last frame
    auto HasMoreFrames() -> bool;

    /// @brief Decode the next frame(s) from media data
    /// The player will only decode new frames when it needs to due to the frame rate of the video.
    auto DecodeAndPlay() -> void;
}
