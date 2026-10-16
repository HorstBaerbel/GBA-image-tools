#include "adpcm_structs.h"

#ifndef __arm__
#include "exception.h"
#endif

namespace Audio
{
        auto AdpcmFrameHeader::write(void *dst, const AdpcmFrameHeader &header) -> void
        {
                static_assert(sizeof(AdpcmFrameHeader) % 4 == 0, "Size of frame header must be a multiple of 4 bytes");
#ifndef __arm__
                REQUIRE(header.uncompressedSize < (1 << 16), std::runtime_error, "Uncompressed size must be < 2^16");
                REQUIRE(header.flags == 0, std::runtime_error, "No flags allowed atm");
                REQUIRE(header.nrOfChannels == 1 || header.nrOfChannels == 2, std::runtime_error, "Number of channels must be 1 or 2");
                REQUIRE(header.pcmBitsPerSample >= 1 && header.pcmBitsPerSample <= 32, std::runtime_error, "Number of PCM bits must be in [1,32]");
                REQUIRE(header.adpcmBitsPerSample >= 3 && header.adpcmBitsPerSample <= 5, std::runtime_error, "Number of ADPCM bits must be in [3,5]");
#endif
                auto flags16 = header.flags;
                flags16 |= header.nrOfChannels << 5;
                flags16 |= header.pcmBitsPerSample << 7;
                flags16 |= header.adpcmBitsPerSample << 13;
                auto dst8 = reinterpret_cast<uint8_t *>(dst);
                dst8[0] = flags16;
                dst8[1] = flags16 >> 8;
                dst8[2] = header.uncompressedSize;
                dst8[3] = header.uncompressedSize >> 8;
        }

        auto AdpcmFrameHeader::read(const void *src) -> AdpcmFrameHeader
        {
                static_assert(sizeof(AdpcmFrameHeader) % 4 == 0, "Size of ADPCM frame header must be a multiple of 4 bytes");

                auto src8 = reinterpret_cast<const uint8_t *>(src);
                const uint16_t data16 = src8[0] | (src8[1] << 8);
                AdpcmFrameHeader header;
                header.flags = (data16 & 0x1F);
                header.nrOfChannels = (data16 & 0x60) >> 5;
                header.pcmBitsPerSample = (data16 & 0x1F80) >> 7;
                header.adpcmBitsPerSample = (data16 & 0xE000) >> 13;
                header.uncompressedSize = src8[2] | (src8[3] << 8);
#ifndef __arm__
                REQUIRE(header.flags == 0, std::runtime_error, "No flags allowed atm");
                REQUIRE(header.nrOfChannels == 1 || header.nrOfChannels == 2, std::runtime_error, "Number of channels must be 1 or 2");
                REQUIRE(header.pcmBitsPerSample >= 1 && header.pcmBitsPerSample <= 32, std::runtime_error, "Number of PCM bits must be in [1,32]");
                REQUIRE(header.adpcmBitsPerSample >= 3 && header.adpcmBitsPerSample <= 5, std::runtime_error, "Number of ADPCM bits must be in [3,5]");
#endif
                return header;
        }
}