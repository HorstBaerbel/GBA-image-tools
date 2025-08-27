#include "adpcm.h"

#include "exception.h"

#include "adpcm-xq/adpcm-lib.h"

namespace Audio
{

    std::vector<uint8_t> ADPCM::FrameHeader::toVector() const
    {
        static_assert(sizeof(FrameHeader) % 4 == 0, "Size of frame header must be a multiple of 4 bytes");
        REQUIRE(uncompressedSize < 2 ^ 16, std::runtime_error, "Uncompressed size must be < 2^16");
        REQUIRE(flags == 0, std::runtime_error, "No flags allowed atm");
        REQUIRE(nrOfChannels == 1 || nrOfChannels == 2, std::runtime_error, "Number of channels must be 1 or 2");
        REQUIRE(pcmBitsPerSample >= 1 && pcmBitsPerSample <= 32, std::runtime_error, "Number of PCM bits must be in [1,32]");
        REQUIRE(adpcmBitsPerSample >= 3 && adpcmBitsPerSample <= 5, std::runtime_error, "Number of ADPCM bits must be in [3,5]");
        std::vector<uint8_t> result(sizeof(FrameHeader));
        auto dataPtr16 = reinterpret_cast<uint16_t *>(result.data());
        *dataPtr16 = flags;
        *dataPtr16 |= nrOfChannels << 5;
        *dataPtr16 |= pcmBitsPerSample << 7;
        *dataPtr16 |= adpcmBitsPerSample << 13;
        dataPtr16++;
        *dataPtr16 = uncompressedSize;
        return result;
    }

    auto ADPCM::FrameHeader::fromVector(const std::vector<uint8_t> &data) -> ADPCM::FrameHeader
    {
        static_assert(sizeof(FrameHeader) % 4 == 0, "Size of frame header must be a multiple of 4 bytes");
        REQUIRE(data.size() >= sizeof(FrameHeader), std::runtime_error, "Data size must be >= " << sizeof(FrameHeader));
        auto dataPtr16 = reinterpret_cast<const uint16_t *>(data.data());
        const auto data16 = *dataPtr16++;
        FrameHeader header;
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

    /// @brief ADPCM-XQ state for (de-)compression
    struct ADPCM::State
    {
        void *context = nullptr;
    };

    ADPCM::ADPCM(Audio::ChannelFormat channelFormat, uint32_t sampleRateHz)
        : m_state(std::make_shared<State>()), m_sampleRateHz(sampleRateHz)
    {
        const auto channelFormatInfo = formatInfo(channelFormat);
        m_nrOfChannels = channelFormatInfo.nrOfChannels;
        REQUIRE(m_nrOfChannels == 1 || m_nrOfChannels == 2, std::runtime_error, "Number of channels must be 1 or 2");
        m_state->context = adpcm_create_context(m_nrOfChannels, static_cast<int>(sampleRateHz), LOOKAHEAD, NOISE_SHAPING_DYNAMIC);
        REQUIRE(m_state->context != nullptr, std::runtime_error, "Failed to allocate ADPCM-XQ context");
    }

    auto ADPCM::encode(const Audio::SampleData &samples, Statistics::Frame::SPtr statistics) -> std::vector<uint8_t>
    {
        REQUIRE(std::holds_alternative<std::vector<int16_t>>(samples), std::runtime_error, "Input sample type must be int16_t");
        // get sample data from frame
        auto &pcmSamples = std::get<std::vector<int16_t>>(samples);
        const auto pcmNrOfSamples = pcmSamples.size() / m_nrOfChannels;
        REQUIRE(m_nrOfChannels == 1 || pcmSamples.size() % 2 == 0, std::runtime_error, "Stereo data must have an even number of samples");
        // generate frame header
        FrameHeader frameHeader;
        frameHeader.flags = 0;
        frameHeader.nrOfChannels = m_nrOfChannels;
        frameHeader.pcmBitsPerSample = 16;
        frameHeader.adpcmBitsPerSample = BITS_PER_SAMPLE;
        frameHeader.uncompressedSize = pcmSamples.size() * sizeof(pcmSamples.front());
        // allocate audio conversion buffer and add frame header
        std::vector<uint8_t> result = frameHeader.toVector();
        const auto adpcmBlockSize = adpcm_sample_count_to_block_size(pcmNrOfSamples, m_nrOfChannels, BITS_PER_SAMPLE);
        REQUIRE(adpcmBlockSize < 2 ^ 16, std::runtime_error, "Block size must be < 2^16");
        result.resize(result.size() + adpcmBlockSize);
        // convert samples to ADPCM format
        auto adpcmDataPtr = result.data() + sizeof(FrameHeader);
        size_t adpcmDataSize = 0;
        adpcm_encode_block_ex(m_state->context, adpcmDataPtr, &adpcmDataSize, pcmSamples.data(), pcmNrOfSamples, BITS_PER_SAMPLE);
        REQUIRE(adpcmDataSize == adpcmBlockSize, std::runtime_error, "Failed to encode block (expected " << adpcmBlockSize << "bytes , got " << adpcmDataSize << " bytes)");
        return result;
    }

    auto ADPCM::decode(const std::vector<uint8_t> &data) -> Audio::SampleData
    {
        REQUIRE(data.size() >= sizeof(FrameHeader), std::runtime_error, "Not enough data to decode");
        // read frame reader
        const auto frameHeader = FrameHeader::fromVector(data);
        // allocate audio conversion buffers
        const auto adpcmDataSize = data.size() - sizeof(FrameHeader);
        const auto adpcmNrOfSamples = adpcm_block_size_to_sample_count(adpcmDataSize, frameHeader.nrOfChannels, BITS_PER_SAMPLE);
        std::vector<int16_t> pcmSamples(adpcmNrOfSamples * frameHeader.nrOfChannels);
        // decode ADPCM samples
        auto adpcmDataPtr = data.data() + sizeof(FrameHeader);
        auto pcmDataPtr = pcmSamples.data();
        const auto pcmNrOfSamples = adpcm_decode_block_ex(pcmDataPtr, adpcmDataPtr, adpcmDataSize, frameHeader.nrOfChannels, BITS_PER_SAMPLE);
        REQUIRE(adpcmNrOfSamples == pcmNrOfSamples, std::runtime_error, "Failed to decode block (expected " << adpcmNrOfSamples << " samples , got " << pcmNrOfSamples << " samples)");
        return pcmSamples;
    }

    ADPCM::~ADPCM()
    {
        if (m_state->context)
        {
            adpcm_free_context(m_state->context);
            m_state->context = nullptr;
        }
    }

}