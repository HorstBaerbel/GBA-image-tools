#pragma once

#include "videostructs.h"

#include <cstdint>

namespace Video
{

    /// @brief Initialize video player for video stream. Call before using any of the other functions
    /// @param videoSrc Video source data
    /// @param scratchPad Intermediate memory for decoding. Can be nullptr if you only have one compression stage. Must be aligned to 4 bytes!
    /// @param scratchPadSize Size of intermediate memory for decoding. Must be a multiple of 4 bytes!
    /// @note The video player uses timer #2 and the matching timer IRQ. Don't use these otherwise!
    auto init(const uint32_t *videoSrc, uint32_t *scratchPad, uint32_t scratchPadSize) -> void;

    /// @brief Set color that screen an scratchpad will be set to when starting playback
    /// @param color
    auto setClearColor(uint16_t color) -> void;

    /// @brief Get video information
    auto getInfo() -> const Video::Info &;

    /// @brief Start playing video. If not stopped, the video will repeat
    auto play() -> void;

    /// @brief Stop playing video
    auto stop() -> void;

    /// @brief Check if there are more in the video
    /// @return True if the video has more frames, false if this is the last frame
    auto hasMoreFrames() -> bool;

    /// @brief Decode a video frame to scratchpad and blit it to dst. If not stopped, the video will repeat.
    /// The player will only decode new frames when it needs to due to the frame rate of the video.
    auto decodeAndBlitFrame(uint32_t *dst) -> void;

}
