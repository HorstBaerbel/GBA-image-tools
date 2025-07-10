#include "samplebuffer.h"

#include "audiohelpers.h"
#include "exception.h"

#include <variant>

namespace Audio
{

    template <typename T>
    auto combineSamples(const SampleData &a, const SampleData &b, ChannelFormat channelFormat) -> std::vector<T>
    {
        REQUIRE(a.index() == b.index(), std::runtime_error, "Sample data must be of same type");
        REQUIRE(std::holds_alternative<std::vector<T>>(a), std::runtime_error, "Bad sample data type in a");
        REQUIRE(std::holds_alternative<std::vector<T>>(b), std::runtime_error, "Bad sample data type in b");
        auto &aSamples = std::get<std::vector<T>>(a);
        auto &bSamples = std::get<std::vector<T>>(b);
        REQUIRE(bSamples.size() != 0, std::runtime_error, "Empty sample data");
        const auto aNrOfSamples = aSamples.size();
        const auto bNrOfSamples = bSamples.size();
        std::vector<T> result;
        result.reserve(aNrOfSamples + bNrOfSamples);
        if (channelFormat == ChannelFormat::Stereo)
        {
            REQUIRE(aNrOfSamples % 2 == 0, std::runtime_error, "Number of samples in a must be divisible by 2");
            REQUIRE(bNrOfSamples % 2 == 0, std::runtime_error, "Number of samples in b must be divisible by 2");
            std::copy(aSamples.cbegin(), std::next(aSamples.cbegin(), aNrOfSamples / 2), std::back_inserter(result));
            std::copy(bSamples.cbegin(), std::next(bSamples.cbegin(), bNrOfSamples / 2), std::back_inserter(result));
            std::copy(std::next(aSamples.cbegin(), aNrOfSamples / 2), aSamples.cend(), std::back_inserter(result));
            std::copy(std::next(bSamples.cbegin(), bNrOfSamples / 2), bSamples.cend(), std::back_inserter(result));
        }
        else
        {
            result = aSamples;
            std::copy(bSamples.cbegin(), bSamples.cend(), std::back_inserter(result));
        }
        return result;
    }

    template <typename T>
    auto extractSamples(SampleData &a, std::size_t nrOfSamplesPerChannel, ChannelFormat channelFormat) -> std::vector<T>
    {
        REQUIRE(std::holds_alternative<std::vector<T>>(a), std::runtime_error, "Bad sample data type in a");
        auto &aSamples = std::get<std::vector<T>>(a);
        const auto aNrOfSamples = aSamples.size();
        REQUIRE(nrOfSamplesPerChannel > 0, std::runtime_error, "Number of extracted samples can not be zero");
        std::vector<T> result;
        std::vector<T> rest;
        auto channel0Start = aSamples.cbegin();
        if (channelFormat == ChannelFormat::Stereo)
        {
            REQUIRE(nrOfSamplesPerChannel * 2 <= aNrOfSamples, std::runtime_error, "Not enough samples in buffer");
            result.reserve(nrOfSamplesPerChannel * 2);
            auto channel1Start = std::next(aSamples.cbegin(), aNrOfSamples / 2);
            // copy samples to result
            std::copy(channel0Start, std::next(channel0Start, nrOfSamplesPerChannel), std::back_inserter(result));
            std::copy(channel1Start, std::next(channel1Start, nrOfSamplesPerChannel), std::back_inserter(result));
            // move rest of buffer to front
            const auto restNrOfSamples = aNrOfSamples - nrOfSamplesPerChannel * 2;
            if (restNrOfSamples > 0)
            {
                rest.reserve(restNrOfSamples);
                std::copy(std::next(channel0Start, nrOfSamplesPerChannel), channel1Start, std::back_inserter(rest));
                std::copy(std::next(channel1Start, nrOfSamplesPerChannel), aSamples.cend(), std::back_inserter(rest));
            }
        }
        else
        {
            REQUIRE(nrOfSamplesPerChannel >= aNrOfSamples, std::runtime_error, "Not enough samples in buffer");
            result.reserve(nrOfSamplesPerChannel);
            // copy samples to result
            std::copy(channel0Start, std::next(channel0Start, nrOfSamplesPerChannel), std::back_inserter(result));
            // move rest of buffer to front
            const auto restNrOfSamples = aNrOfSamples - nrOfSamplesPerChannel;
            if (restNrOfSamples > 0)
            {
                rest.reserve(restNrOfSamples);
                std::copy(std::next(channel0Start, nrOfSamplesPerChannel), aSamples.cend(), std::back_inserter(rest));
            }
        }
        a = rest;
        return result;
    }

    SampleBuffer::SampleBuffer(ChannelFormat channelFormat, uint32_t sampleRateHz, SampleFormat sampleFormat)
        : m_channelFormat(channelFormat), m_sampleRateHz(sampleRateHz), m_sampleFormat(sampleFormat)
    {
        REQUIRE(m_channelFormat != ChannelFormat::Unknown, std::runtime_error, "Bad channel format");
        REQUIRE(m_sampleRateHz != 0, std::runtime_error, "Bad sample rate");
        REQUIRE(m_sampleFormat != SampleFormat::Unknown, std::runtime_error, "Bad sample format");
        m_samples = AudioHelpers::createSampleBuffer(m_sampleFormat);
    }

    auto SampleBuffer::nrOfSamplesPerChannel() const -> std::size_t
    {
        auto bufferSize = std::visit([](const auto &samples)
                                     { return samples.size(); }, m_samples);
        return m_channelFormat == ChannelFormat::Stereo ? bufferSize / 2 : bufferSize;
    }

    auto SampleBuffer::push_back(const Frame &frame) -> void
    {
        REQUIRE(m_channelFormat == frame.info.channelFormat, std::runtime_error, "Unexpected frame channel format");
        REQUIRE(m_sampleRateHz == frame.info.sampleRateHz, std::runtime_error, "Unexpected sample rate");
        REQUIRE(m_sampleFormat == frame.info.sampleFormat, std::runtime_error, "Unexpected frame sample format");
        REQUIRE(m_samples.index() == frame.data.index(), std::runtime_error, "Unexpected frame data type");
        switch (m_sampleFormat)
        {
        case Audio::SampleFormat::Signed8:
            m_samples = combineSamples<int8_t>(m_samples, frame.data, m_channelFormat);
            break;
        case Audio::SampleFormat::Unsigned8:
            m_samples = combineSamples<uint8_t>(m_samples, frame.data, m_channelFormat);
            break;
        case Audio::SampleFormat::Signed16:
            m_samples = combineSamples<int16_t>(m_samples, frame.data, m_channelFormat);
            break;
        case Audio::SampleFormat::Unsigned16:
            m_samples = combineSamples<uint16_t>(m_samples, frame.data, m_channelFormat);
            break;
        case Audio::SampleFormat::Float32:
            m_samples = combineSamples<float>(m_samples, frame.data, m_channelFormat);
            break;
        default:
            THROW(std::runtime_error, "Bad sample format");
        }
    }

    auto SampleBuffer::pop_front(std::size_t nrOfSamplesPerChannel) -> Frame
    {
        Frame frame;
        frame.info.channelFormat = m_channelFormat;
        frame.info.sampleRateHz = m_sampleRateHz;
        frame.info.sampleFormat = m_sampleFormat;
        switch (m_sampleFormat)
        {
        case Audio::SampleFormat::Signed8:
            frame.data = extractSamples<int8_t>(m_samples, nrOfSamplesPerChannel, m_channelFormat);
            break;
        case Audio::SampleFormat::Unsigned8:
            frame.data = extractSamples<uint8_t>(m_samples, nrOfSamplesPerChannel, m_channelFormat);
            break;
        case Audio::SampleFormat::Signed16:
            frame.data = extractSamples<int16_t>(m_samples, nrOfSamplesPerChannel, m_channelFormat);
            break;
        case Audio::SampleFormat::Unsigned16:
            frame.data = extractSamples<uint16_t>(m_samples, nrOfSamplesPerChannel, m_channelFormat);
            break;
        case Audio::SampleFormat::Float32:
            frame.data = extractSamples<float>(m_samples, nrOfSamplesPerChannel, m_channelFormat);
            break;
        default:
            THROW(std::runtime_error, "Bad sample format");
        }
        return frame;
    }
}
