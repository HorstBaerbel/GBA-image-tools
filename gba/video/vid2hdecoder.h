#pragma once

#include "vid2hio.h"

#include <cstdint>
#include <tuple>

namespace Media
{

    /// @brief Decode video frame to scratchPad, possibly using a scratchPad as intermediate memory
    /// @param scratchPad8 Memory for decoding in bytes. Must be able to hold a full decoded frame AND intermediate memory. Must be aligned to 4 bytes!
    /// @param scratchPadSize8 Size of memory for decoding in bytes. Must be a multiple of 4 bytes!
    /// @param vramPtr8 Pointer to start of VRAM destination (for direct copy or getting previous frames)
    /// @param vramLineStride8 Line stride for one line in VRAM in bytes
    /// @param info Static video info
    /// @param frame Video frame to decode
    /// @return Returns pointer to decoded image and size of decoded samples in bytes
    auto DecodeVideo(uint32_t *scratchPad8, const uint32_t scratchPadSize8, uint8_t *vramPtr8, const uint32_t vramLineStride8, const IO::Vid2h::Info &info, const IO::Vid2h::Frame &frame) -> std::pair<const uint32_t *, uint32_t>;

    /// @brief Decode audio frame to scratchPad, possibly using a scratchPad as intermediate memory
    /// @param scratchPad Memory for decoding in bytes. Must be able to hold a full decoded frame AND intermediate memory. Must be aligned to 4 bytes!
    /// @param scratchPadSize Size of memory for decoding in bytes. Must be a multiple of 4 bytes!
    /// @param info Static video info
    /// @param frame Audio frame to decode
    /// @return Returns pointer to decoded samples and size of decoded samples in bytes
    auto DecodeAudio(uint32_t *scratchPad8, const uint32_t scratchPadSize8, const IO::Vid2h::Info &info, const IO::Vid2h::Frame &frame) -> std::pair<const uint32_t *, uint32_t>;
}
