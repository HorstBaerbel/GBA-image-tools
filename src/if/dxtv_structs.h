#pragma once

#include <cstdint>

namespace Video
{
    /// @brief Frame header for one DTXV frame
    struct DxtvFrameHeader
    {
        uint8_t frameFlags = 0;         // General frame flags, e.g. FRAME_IS_PFRAME or FRAME_KEEP
        uint32_t uncompressedSize : 24; // Uncompressed size of data in bytes

        static auto write(uint32_t *dst, const DxtvFrameHeader &header) -> void;
        static auto read(const uint32_t *src) -> DxtvFrameHeader;
    } __attribute__((aligned(4), packed));
}
