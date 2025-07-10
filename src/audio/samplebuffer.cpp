#include "samplebuffer.h"

#include "audiohelpers.h"
#include "exception.h"

#include <variant>

namespace Audio
{

    template <typename T>
    auto combineSamples(const SampleData &a, const SampleData &b, ChannelFormat channelFormat) -> std::vector<T>
    {
        auto &currSamples = std::get<std::vector<T>>(a);
        auto &newSamples = std::get<std::vector<T>>(b);
        REQUIRE(newSamples.size() != 0, std::runtime_error, "Empty sample data");
        const auto currSampleSize = currSamples.size();
        const auto newSampleSize = newSamples.size();
        std::vector<T> result;
        result.reserve(currSampleSize + newSampleSize);
        if (channelFormat == ChannelFormat::Stereo)
        {
            REQUIRE(newSamples.size() % 2 == 0, std::runtime_error, "Sample size must be divisible by 2");
            std::copy(currSamples.cbegin(), std::next(currSamples.cbegin(), currSampleSize / 2), std::back_inserter(result));
            std::copy(newSamples.cbegin(), std::next(newSamples.cbegin(), newSampleSize / 2), std::back_inserter(result));
            std::copy(std::next(currSamples.cbegin(), currSampleSize / 2), currSamples.cend(), std::back_inserter(result));
            std::copy(std::next(newSamples.cbegin(), newSampleSize / 2), newSamples.cend(), std::back_inserter(result));
        }
        else
        {
            result = currSamples;
            std::copy(newSamples.cbegin(), newSamples.cend(), std::back_inserter(result));
        }
        return result;
    }

    template <typename T>
    auto extractSamples(SampleData &a, std::size_t nrOfSamplesPerChannel, ChannelFormat channelFormat) -> std::vector<T>
    {
        auto &currSamples = std::get<std::vector<T>>(a);
        const auto currNrOfSamples = currSamples.size();
        REQUIRE(nrOfSamplesPerChannel > 0, std::runtime_error, "Number of requested samples can not be zero");
        std::vector<T> result;
        auto channel0Start = currSamples.cbegin();
        if (channelFormat == ChannelFormat::Stereo)
        {
            REQUIRE(nrOfSamplesPerChannel % 2 == 0, std::runtime_error, "Number of requested samples must be divisible by 2");
            REQUIRE(nrOfSamplesPerChannel * 2 <= currNrOfSamples, std::runtime_error, "Not enough samples in buffer");
            result.reserve(nrOfSamplesPerChannel * 2);
            auto channel1Start = std::next(currSamples.cbegin(), currNrOfSamples / 2);
            // copy samples to result
            std::copy(channel0Start, std::next(channel0Start, nrOfSamplesPerChannel), std::back_inserter(result));
            std::copy(channel1Start, std::next(channel1Start, nrOfSamplesPerChannel), std::back_inserter(result));
            // move rest of buffer to front
            const auto restNrOfSamples = currNrOfSamples - nrOfSamplesPerChannel * 2;
            if (restNrOfSamples > 0)
            {
                std::copy_backward(std::next(channel0Start, nrOfSamplesPerChannel), channel1Start, currSamples.begin());
                std::copy_backward(std::next(channel1Start, nrOfSamplesPerChannel), currSamples.cend(), std::next(currSamples.begin(), restNrOfSamples / 2));
            }
            currSamples.resize(restNrOfSamples);
        }
        else
        {
            REQUIRE(nrOfSamplesPerChannel >= currNrOfSamples, std::runtime_error, "Not enough samples in buffer");
            result.reserve(nrOfSamplesPerChannel);
            // copy samples to result
            std::copy(channel0Start, std::next(channel0Start, nrOfSamplesPerChannel), std::back_inserter(result));
            // move rest of buffer to front
            const auto restNrOfSamples = currNrOfSamples - nrOfSamplesPerChannel;
            if (restNrOfSamples > 0)
            {
                std::copy_backward(std::next(channel0Start, nrOfSamplesPerChannel), currSamples.cend(), currSamples.begin());
            }
            currSamples.resize(restNrOfSamples);
        }
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
