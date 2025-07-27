#pragma once

#include "vid2hio.h"

#include <cstdint>

namespace Media
{

    /// @brief Initialize video player for video stream. Call before using any of the other functions
    /// @param videoSrc Video source data
    /// @param scratchPad Intermediate memory for decoding. Can be nullptr if you only have one compression stage. Must be aligned to 4 bytes!
    /// @param scratchPadSize Size of intermediate memory for decoding. Must be a multiple of 4 bytes!
    /// @param audioData Memory for storing audio sample data. Must be in IWRAM. Must be aligned to 4 bytes!
    /// @param audioDataSize Size of memory for storing audio sample data. Must be a multiple of 4 bytes!
    /// @note Will use Timer 0, 1, 2, IRQ 1, 2 and DMA 1 (mono) or DMA 1 + 2 (stereo). Also modifies sound registers, especially REG_SOUNDCNT_X
    auto Init(const uint32_t *videoSrc, uint32_t *scratchPad, uint32_t scratchPadSize, uint32_t *audioData, uint32_t audioDataSize) -> void;

    /// @brief Set color that screen an scratchpad will be set to when starting playback
    /// @param color
    auto SetClearColor(uint16_t color) -> void;

    /// @brief Get video information
    auto GetInfo() -> const IO::Vid2h::Info &;

    /// @brief Start playing video. If not stopped, the video will repeat
    auto Play() -> void;

    /// @brief Stop playing video
    auto Stop() -> void;

    /// @brief Check if there are more in the video
    /// @return True if the video has more frames, false if this is the last frame
    auto HasMoreFrames() -> bool;

    /// @brief Decode a video frame to scratchpad and blit it to dst. If not stopped, the video will repeat.
    /// The player will only decode new frames when it needs to due to the frame rate of the video.
    auto DecodeAndBlitFrame(uint32_t *dst) -> void;
}
