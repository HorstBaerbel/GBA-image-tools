#include "resampler.h"

#include "exception.h"

extern "C"
{
#include <inttypes.h>
#include <libavutil/channel_layout.h>
#include <libavutil/samplefmt.h>
#include <libswresample/swresample.h>
}

#include <cstring>
#include <algorithm>
#include <vector>

namespace Audio
{

    static const AVChannelLayout MonoLayout = AV_CHANNEL_LAYOUT_MONO;
    static const AVChannelLayout StereoLayout = AV_CHANNEL_LAYOUT_STEREO;

    /// @brief FFmpeg state for a audio resampler
    struct Resampler::State
    {
        SwrContext *swrContext = nullptr; // Audio format conversion context
        AVChannelLayout outLayout = MonoLayout;
        AVSampleFormat outFormat = AV_SAMPLE_FMT_NONE;
        uint8_t *outData[2] = {nullptr, nullptr}; // Stereo audio conversion output sample buffer
        uint32_t outDataMaxSamples = 0;           // Audio conversion output sample buffer size
        int outLinesize = 0;
    };

    auto ToAvChannelLayout(ChannelFormat format) -> AVChannelLayout
    {
        REQUIRE(format != ChannelFormat::Unknown, std::runtime_error, "Bad channel format");
        REQUIRE(format == ChannelFormat::Mono || format == ChannelFormat::Stereo, std::runtime_error, "Unsupported channel format: " << static_cast<uint32_t>(format));
        return format == ChannelFormat::Mono ? MonoLayout : StereoLayout;
    }

    auto ToAvSampleFormat(SampleFormat format) -> AVSampleFormat
    {
        REQUIRE(format != SampleFormat::Unknown, std::runtime_error, "Bad sample format");
        switch (format)
        {
        case SampleFormat::Signed8P:
            // FFmepg supports no S8P, so we converted to U8P
            return AV_SAMPLE_FMT_U8P;
            break;
        case SampleFormat::Unsigned8P:
            return AV_SAMPLE_FMT_U8P;
            break;
        case SampleFormat::Signed16P:
            return AV_SAMPLE_FMT_S16P;
            break;
        case SampleFormat::Unsigned16P:
            // FFmepg supports no U16P, so we converted to S16P
            return AV_SAMPLE_FMT_S16P;
            break;
        case SampleFormat::Float32P:
            return AV_SAMPLE_FMT_FLTP;
            break;
        default:
            THROW(std::runtime_error, "Unsupported sample format: " << static_cast<uint32_t>(format));
        }
    }

    template <typename T>
    auto rawBufferToVector(const uint8_t *data, uint32_t rawBufferSize) -> std::vector<T>
    {
        std::vector<T> v(rawBufferSize / sizeof(T));
        std::memcpy(v.data(), data, rawBufferSize);
        return v;
    }

    Resampler::Resampler(ChannelFormat inChannelFormat, uint32_t inSampleRateHz,
                         ChannelFormat outChannelFormat, uint32_t outSampleRateHz, SampleFormat outSampleFormat)
        : m_state(std::make_shared<State>()),
          m_inChannelFormat(inChannelFormat), m_inSampleRateHz(inSampleRateHz),
          m_outChannelFormat(outChannelFormat), m_outSampleRateHz(outSampleRateHz), m_outSampleFormat(outSampleFormat)
    {
        // get FFmpeg formats from input data
        const auto inLayout = ToAvChannelLayout(m_inChannelFormat);
        const auto inFormat = AV_SAMPLE_FMT_S16P;
        m_state->outLayout = ToAvChannelLayout(m_outChannelFormat);
        m_state->outFormat = ToAvSampleFormat(m_outSampleFormat);
        // allocate resampler context
        auto swrAllocResult = swr_alloc_set_opts2(&m_state->swrContext, &m_state->outLayout, m_state->outFormat, outSampleRateHz, &inLayout, inFormat, inSampleRateHz, 0, nullptr);
        REQUIRE(swrAllocResult == 0 && m_state->swrContext != nullptr, std::runtime_error, "Failed to allocate audio swresampler context: " << swrAllocResult);
        // initialize resampler context
        auto swrInitResult = swr_init(m_state->swrContext);
        REQUIRE(swrInitResult == 0, std::runtime_error, "Failed to init audio swresampler context: " << swrInitResult);
    }

    auto Resampler::getOutputFormat() const -> FrameInfo
    {
        FrameInfo info;
        info.channelFormat = m_outChannelFormat;
        info.sampleFormat = m_outSampleFormat;
        info.sampleRateHz = m_outSampleRateHz;
        return info;
    }

    auto Resampler::resample(const Frame &inFrame) -> Frame
    {
        REQUIRE(std::holds_alternative<std::vector<int16_t>>(inFrame.data), std::runtime_error, "Input sample type must be int16_t");
        REQUIRE(inFrame.info.sampleRateHz == m_inSampleRateHz, std::runtime_error, "Frame sample rate does not match initial sample rate");
        REQUIRE(inFrame.info.channelFormat == m_inChannelFormat, std::runtime_error, "Frame channel format does not match initial channel format");
        // get sample data from frame
        auto &inSamples = std::get<std::vector<int16_t>>(inFrame.data);
        REQUIRE(m_inChannelFormat == ChannelFormat::Mono || inSamples.size() % 2 == 0, std::runtime_error, "Stereo data must have an even number of samples");
        // check if we need to reallocate audio conversion output buffers
        const auto inDataNrOfSamples = m_inChannelFormat == ChannelFormat::Mono ? inSamples.size() : inSamples.size() / 2;
        const auto maxNrOfOutputSamples = swr_get_out_samples(m_state->swrContext, inDataNrOfSamples);
        REQUIRE(maxNrOfOutputSamples >= 0, std::runtime_error, "Failed to get maximum number of output samples: " << maxNrOfOutputSamples);
        if (maxNrOfOutputSamples > m_state->outDataMaxSamples)
        {
            av_freep(&m_state->outData[0]);
            m_state->outDataMaxSamples = 0;
            const auto allocateResult = av_samples_alloc(m_state->outData, &m_state->outLinesize, m_state->outLayout.nb_channels, maxNrOfOutputSamples, m_state->outFormat, 1);
            REQUIRE(allocateResult >= 0, std::runtime_error, "Failed to allocate audio conversion buffer: " << allocateResult);
            m_state->outDataMaxSamples = maxNrOfOutputSamples;
        }
        // create pointers to planar input channels
        auto inChannel0Ptr = reinterpret_cast<const uint8_t *>(inSamples.data());
        auto inChannel1Ptr = reinterpret_cast<const uint8_t *>(inSamples.data() + inSamples.size() / 2);
        const uint8_t *inData[2] = {inChannel0Ptr, inChannel1Ptr};
        // convert audio format using FFmpeg sw resampler
        const auto nrOfSamplesConverted = swr_convert(m_state->swrContext, m_state->outData, m_state->outDataMaxSamples, inData, inDataNrOfSamples);
        REQUIRE(nrOfSamplesConverted >= 0, std::runtime_error, "Failed to convert audio data: " << nrOfSamplesConverted);
        // get size of a raw, combined, byte buffer needed to hold all sample data of all channels
        const auto convertedRawBufferSize = av_samples_get_buffer_size(&m_state->outLinesize, m_state->outLayout.nb_channels, nrOfSamplesConverted, m_state->outFormat, 1);
        REQUIRE(convertedRawBufferSize >= 0, std::runtime_error, "Failed to get number of audio samples output to buffer: " << convertedRawBufferSize);
        // convert output sample format from AV formats
        SampleData outSamples;
        switch (m_outSampleFormat)
        {
        case SampleFormat::Signed8P:
        {
            // FFmepg supports no S8P, so we converted to U8P
            auto dataU8 = rawBufferToVector<uint8_t>(m_state->outData[0], convertedRawBufferSize);
            // correctly convert to int8_t
            std::vector<int8_t> dataI8;
            dataI8.reserve(dataU8.size());
            std::transform(dataU8.cbegin(), dataU8.cend(), std::back_inserter(dataI8), [](auto v)
                           { return static_cast<int8_t>(static_cast<int32_t>(v) - 128); });
            outSamples = dataI8;
            break;
        }
        case SampleFormat::Unsigned8P:
            outSamples = rawBufferToVector<uint8_t>(m_state->outData[0], convertedRawBufferSize);
            break;
        case SampleFormat::Signed16P:
            outSamples = rawBufferToVector<int16_t>(m_state->outData[0], convertedRawBufferSize);
            break;
        case SampleFormat::Unsigned16P:
        {
            // FFmepg supports no U16P, so we converted to S16P
            auto dataS16 = rawBufferToVector<int16_t>(m_state->outData[0], convertedRawBufferSize);
            // correctly convert to uint16_t
            std::vector<uint16_t> dataU16;
            dataU16.reserve(dataS16.size());
            std::transform(dataS16.cbegin(), dataS16.cend(), std::back_inserter(dataU16), [](auto v)
                           { return static_cast<uint16_t>(static_cast<int32_t>(v) + 32768); });
            outSamples = dataU16;
            break;
        }
        case SampleFormat::Float32P:
            outSamples = rawBufferToVector<float>(m_state->outData[0], convertedRawBufferSize);
            break;
        default:
            THROW(std::runtime_error, "Bad output sample format");
        }
        // build output frame
        Frame outFrame;
        outFrame.index = inFrame.index;
        outFrame.fileName = inFrame.fileName;
        outFrame.info.compressed = inFrame.info.compressed;
        outFrame.info.maxMemoryNeeded = inFrame.info.maxMemoryNeeded;
        outFrame.info.channelFormat = m_outChannelFormat;
        outFrame.info.sampleFormat = m_outSampleFormat;
        outFrame.info.sampleRateHz = m_outSampleRateHz;
        outFrame.data = outSamples;
        return outFrame;
    }

    Resampler::~Resampler()
    {
        if (m_state->outData[0])
        {
            av_freep(&m_state->outData[0]);
            m_state->outDataMaxSamples = 0;
        }
        if (m_state->swrContext)
        {
            swr_free(&m_state->swrContext);
            m_state->swrContext = nullptr;
        }
    }
}
