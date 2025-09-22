#include "adpcm.h"

#include "exception.h"
#include "if/adpcm_constants.h"
#include "if/adpcm_structs.h"

#include "adpcm-xq/adpcm-lib.h"

namespace Audio
{
    /// @brief ADPCM-XQ state for (de-)compression
    struct Adpcm::State
    {
        void *context[2] = {nullptr, nullptr}; // ADPCM-XQ state per channel
    };

    Adpcm::Adpcm(Audio::ChannelFormat channelFormat, uint32_t sampleRateHz)
        : m_state(std::make_shared<State>()), m_sampleRateHz(sampleRateHz)
    {
        const auto channelFormatInfo = formatInfo(channelFormat);
        m_nrOfChannels = channelFormatInfo.nrOfChannels;
        REQUIRE(m_nrOfChannels == 1 || m_nrOfChannels == 2, std::runtime_error, "Number of channels must be 1 or 2");
        for (uint32_t ch = 0; ch < m_nrOfChannels; ++ch)
        {
            m_state->context[ch] = adpcm_create_context(1, static_cast<int>(sampleRateHz), AdpcmConstants::LOOKAHEAD, NOISE_SHAPING_DYNAMIC);
            REQUIRE(m_state->context != nullptr, std::runtime_error, "Failed to allocate ADPCM-XQ context for channel " << ch);
        }
    }

    auto Adpcm::encode(const Audio::SampleData &samples, Statistics::Frame::SPtr statistics) -> std::vector<uint8_t>
    {
        REQUIRE(std::holds_alternative<std::vector<int16_t>>(samples), std::runtime_error, "Input sample type must be int16_t");
        // get sample data from frame
        auto &pcmSamples = std::get<std::vector<int16_t>>(samples);
        const auto pcmNrOfSamples = pcmSamples.size() / m_nrOfChannels;
        REQUIRE(m_nrOfChannels == 1 || pcmSamples.size() % 2 == 0, std::runtime_error, "Stereo data must have an even number of samples");
        const auto pcmDataSize = pcmSamples.size() * sizeof(pcmSamples.front());
        REQUIRE(pcmDataSize < (1 << 16), std::runtime_error, "PCM data size must be < 2^16");
        // generate frame header
        AdpcmFrameHeader frameHeader;
        frameHeader.flags = 0;
        frameHeader.nrOfChannels = m_nrOfChannels;
        frameHeader.pcmBitsPerSample = 16;
        frameHeader.adpcmBitsPerSample = AdpcmConstants::BITS_PER_SAMPLE;
        frameHeader.uncompressedSize = pcmDataSize;
        // allocate audio conversion buffer and add frame header
        std::vector<uint8_t> result(sizeof(AdpcmFrameHeader));
        AdpcmFrameHeader::write(reinterpret_cast<uint32_t *>(result.data()), frameHeader);
        const auto adpcmChannelBlockSize = adpcm_sample_count_to_block_size(pcmNrOfSamples, 1, AdpcmConstants::BITS_PER_SAMPLE);
        result.resize(result.size() + adpcmChannelBlockSize * m_nrOfChannels);
        // convert samples to ADPCM format one channel at a time
        auto pcmChannelDataPtr = pcmSamples.data();
        auto adpcmChannelDataPtr = result.data() + sizeof(AdpcmFrameHeader);
        for (uint32_t ch = 0; ch < m_nrOfChannels; ++ch)
        {
            size_t adpcmConvertedDataSize = 0;
            adpcm_encode_block_ex(m_state->context[ch], adpcmChannelDataPtr, &adpcmConvertedDataSize, pcmChannelDataPtr, pcmNrOfSamples, AdpcmConstants::BITS_PER_SAMPLE);
            REQUIRE(adpcmConvertedDataSize == adpcmChannelBlockSize, std::runtime_error, "Failed to encode channel " << ch << " (expected " << adpcmChannelBlockSize << " bytes , got " << adpcmConvertedDataSize << " bytes)");
            pcmChannelDataPtr += pcmNrOfSamples;
            adpcmChannelDataPtr += adpcmChannelBlockSize;
        }
        return result;
    }

    auto Adpcm::decode(const std::vector<uint8_t> &data) -> Audio::SampleData
    {
        REQUIRE(data.size() >= sizeof(AdpcmFrameHeader), std::runtime_error, "Not enough data to decode");
        // read frame reader
        const AdpcmFrameHeader frameHeader = AdpcmFrameHeader::read(reinterpret_cast<const uint32_t *>(data.data()));
        // allocate audio conversion buffers
        const auto adpcmDataSize = data.size() - sizeof(AdpcmFrameHeader);
        const auto adpcmChannelBlockSize = adpcmDataSize / frameHeader.nrOfChannels;
        const auto adpcmChannelNrOfSamples = adpcm_block_size_to_sample_count(adpcmChannelBlockSize, 1, AdpcmConstants::BITS_PER_SAMPLE);
        std::vector<int16_t> pcmSamples(adpcmChannelNrOfSamples * frameHeader.nrOfChannels);
        // decode ADPCM samples
        auto adpcmChannelDataPtr = data.data() + sizeof(AdpcmFrameHeader);
        auto pcmChannelDataPtr = pcmSamples.data();
        for (uint32_t ch = 0; ch < frameHeader.nrOfChannels; ++ch)
        {
            const auto pcmChannelNrOfSamples = adpcm_decode_block_ex(pcmChannelDataPtr, adpcmChannelDataPtr, adpcmChannelBlockSize, 1, AdpcmConstants::BITS_PER_SAMPLE);
            REQUIRE(adpcmChannelNrOfSamples == pcmChannelNrOfSamples, std::runtime_error, "Failed to decode channel " << ch << " (expected " << adpcmChannelNrOfSamples << " samples , got " << pcmChannelNrOfSamples << " samples)");
            pcmChannelDataPtr += pcmChannelNrOfSamples;
            adpcmChannelDataPtr += adpcmChannelBlockSize;
        }
        return pcmSamples;
    }

    Adpcm::~Adpcm()
    {
        if (m_state->context[0])
        {
            adpcm_free_context(m_state->context[0]);
            m_state->context[0] = nullptr;
        }
        if (m_state->context[1])
        {
            adpcm_free_context(m_state->context[1]);
            m_state->context[1] = nullptr;
        }
    }

}