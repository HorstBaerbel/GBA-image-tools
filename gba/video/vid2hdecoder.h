#pragma once

#include "vid2hio.h"

#include <cstdint>
#include <tuple>

namespace Media
{

    /// @brief Decode video frame to scratchPad, possibly using a scratchPad as intermediate memory
    /// @param scratchPad Memory for decoding in bytes. Must be able to hold a full decoded frame AND intermediate memory. Must be aligned to 4 bytes!
    /// @param scratchPadSize Size of memory for decoding in bytes. Must be a multiple of 4 bytes!
    /// @param info Static video info
    /// @param frameData Video frame to decode
    /// @return Returns pointer to decoded frame
    auto DecodeVideo(uint32_t *scratchPad, uint32_t scratchPadSize, const IO::Vid2h::Info &info, const uint32_t *frameData) -> const uint32_t *;

    /// @brief Decode audio frame to scratchPad, possibly using a scratchPad as intermediate memory
    /// @param scratchPad Memory for decoding in bytes. Must be able to hold a full decoded frame AND intermediate memory. Must be aligned to 4 bytes!
    /// @param scratchPadSize Size of memory for decoding in bytes. Must be a multiple of 4 bytes!
    /// @param info Static video info
    /// @param frameData Audio frame to decode
    /// @return Returns pointer to decoded samples and size of decoded samples in bytes
    auto DecodeAudio(uint32_t *scratchPad, uint32_t scratchPadSize, const IO::Vid2h::Info &info, const uint32_t *frameData) -> std::pair<const uint32_t *, uint32_t>;
}
