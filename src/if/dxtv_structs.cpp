#include "dxtv_structs.h"

namespace Video
{

    auto DxtvFrameHeader::write(uint32_t *dst, const DxtvFrameHeader &header) -> void
    {
        static_assert(sizeof(DxtvFrameHeader) % 4 == 0, "Size of DXTV frame header must be a multiple of 4 bytes");
        *dst = (header.uncompressedSize << 8) | static_cast<uint32_t>(header.frameFlags);
    }

    auto DxtvFrameHeader::read(const uint32_t *src) -> DxtvFrameHeader
    {
        static_assert(sizeof(DxtvFrameHeader) % 4 == 0, "Size of DXTV frame header must be a multiple of 4 bytes");
        DxtvFrameHeader header;
        header.frameFlags = *src & 0xFF;
        header.uncompressedSize = (*src & 0xFFFFFF) >> 8;
        return header;
    }
}