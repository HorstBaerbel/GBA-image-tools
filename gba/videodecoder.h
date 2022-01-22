#pragma once

#include "videostructs.h"

namespace Video
{

    /// @brief Decode frame to finalDest, possibly using a scratchPad as intermediate memory
    /// @param finalDst Final destination for data. Data will be copied there in the final decoding pass. Can be the same as scratchPad. Must be aligned to 4 bytes!
    /// @param scratchPad Intermediate memory for decoding. Can be nullptr if you only have one compression stage. Must be aligned to 4 bytes!
    /// @param scratchPadSize Size of intermediate memory for decoding. Must be a multiple of 4 bytes!
    /// @param info Static video info
    /// @param frame Video frame to decode
    void decode(uint32_t *finalDst, uint32_t *scratchPad, uint32_t scratchPadSize, const Info &info, const Frame &frame);

}
