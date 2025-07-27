#pragma once

#include "vid2hio.h"

namespace Media
{

    /// @brief Decode frame to scratchPad, possibly using a scratchPad as intermediate memory
    /// @param scratchPad Memory for decoding in bytes. Must be able to hold a full decoded frame AND intermediate memory. Must be aligned to 4 bytes!
    /// @param scratchPadSize Size of memory for decoding in bytes. Must be a multiple of 4 bytes!
    /// @param info Static video info
    /// @param frame Video frame to decode
    /// @return Returns pointer to decoded frame
    auto DecodeVideo(uint32_t *scratchPad, uint32_t scratchPadSize, const IO::Vid2h::Info &info, const IO::Vid2h::Frame &frame) -> const uint32_t *;
}
