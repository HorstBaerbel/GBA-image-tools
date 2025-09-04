#pragma once

#include <cstdint>

namespace Image
{
    /// @brief Frame header for one DTXV frame
    struct FrameHeader
    {
        uint8_t frameFlags = 0;             // General frame flags, e.g. FRAME_IS_PFRAME or FRAME_KEEP
        uint32_t uncompressedSize : 24 = 0; // Uncompressed size of data in bytes

        std::vector<uint8_t> toVector() const;
        static auto fromVector(const std::vector<uint8_t> &data) -> FrameHeader;
    } __attribute__((aligned(4), packed));
}
