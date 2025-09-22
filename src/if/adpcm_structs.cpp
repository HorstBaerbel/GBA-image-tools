#include "adpcm_structs.h"

#include "exception.h"

namespace Audio
{

    std::vector<uint8_t> AdpcmFrameHeader::toVector() const
    {
        static_assert(sizeof(AdpcmFrameHeader) % 4 == 0, "Size of frame header must be a multiple of 4 bytes");
        REQUIRE(uncompressedSize < (1 << 16), std::runtime_error, "Uncompressed size must be < 2^16");
        REQUIRE(flags == 0, std::runtime_error, "No flags allowed atm");
        REQUIRE(nrOfChannels == 1 || nrOfChannels == 2, std::runtime_error, "Number of channels must be 1 or 2");
        REQUIRE(pcmBitsPerSample >= 1 && pcmBitsPerSample <= 32, std::runtime_error, "Number of PCM bits must be in [1,32]");
        REQUIRE(adpcmBitsPerSample >= 3 && adpcmBitsPerSample <= 5, std::runtime_error, "Number of ADPCM bits must be in [3,5]");
        std::vector<uint8_t> result(sizeof(AdpcmFrameHeader));
        auto dataPtr16 = reinterpret_cast<uint16_t *>(result.data());
        *dataPtr16 = flags;
        *dataPtr16 |= nrOfChannels << 5;
        *dataPtr16 |= pcmBitsPerSample << 7;
        *dataPtr16 |= adpcmBitsPerSample << 13;
        dataPtr16++;
        *dataPtr16 = uncompressedSize;
        return result;
    }

    auto AdpcmFrameHeader::fromVector(const std::vector<uint8_t> &data) -> AdpcmFrameHeader
    {
        static_assert(sizeof(AdpcmFrameHeader) % 4 == 0, "Size of ADPCM frame header must be a multiple of 4 bytes");
        REQUIRE(data.size() >= sizeof(AdpcmFrameHeader), std::runtime_error, "Data size must be >= " << sizeof(AdpcmFrameHeader));
        auto dataPtr16 = reinterpret_cast<const uint16_t *>(data.data());
        const auto data16 = *dataPtr16++;
        AdpcmFrameHeader header;
        header.flags = (data16 & 0x1F);
        header.nrOfChannels = (data16 & 0x60) >> 5;
        header.pcmBitsPerSample = (data16 & 0x1F80) >> 7;
        header.pcmBitsPerSample = (data16 & 0xE000) >> 13;
        header.uncompressedSize = *dataPtr16;
        REQUIRE(header.flags == 0, std::runtime_error, "No flags allowed atm");
        REQUIRE(header.nrOfChannels == 1 || header.nrOfChannels == 2, std::runtime_error, "Number of channels must be 1 or 2");
        REQUIRE(header.pcmBitsPerSample >= 1 && header.pcmBitsPerSample <= 32, std::runtime_error, "Number of PCM bits must be in [1,32]");
        REQUIRE(header.adpcmBitsPerSample >= 3 && header.adpcmBitsPerSample <= 5, std::runtime_error, "Number of ADPCM bits must be in [3,5]");
        return header;
    }

}