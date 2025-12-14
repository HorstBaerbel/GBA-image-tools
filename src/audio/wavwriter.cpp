#include "wavwriter.h"

#include "audio/audiohelpers.h"
#include "exception.h"

#include <filesystem>
#include <fstream>

namespace IO
{
    auto WavWriter::open(const std::string &filePath) -> void
    {
        static_assert(sizeof(WavHeader) == 44);
        REQUIRE(!filePath.empty(), std::runtime_error, "filePath must contain a file name");
        // open input file
        auto fileWav = std::ofstream(filePath, std::ios::out | std::ios::trunc | std::ios::binary);
        REQUIRE(fileWav.is_open() && !fileWav.fail(), std::runtime_error, "Failed to open " << filePath << " for writing");
        m_os = std::move(fileWav);
    }

    auto WavWriter::writeFrame(const Audio::Frame &frame) -> void
    {
        if (m_isFirstFrame)
        {
            REQUIRE(frame.info.channelFormat != Audio::ChannelFormat::Unknown, std::runtime_error, "Bad audio channel format");
            REQUIRE(frame.info.sampleRateHz > 0 && frame.info.sampleRateHz <= 48000, std::runtime_error, "Bad audio sample rate " << frame.info.sampleRateHz << " Hz");
            REQUIRE(frame.info.sampleFormat != Audio::SampleFormat::Unknown, std::runtime_error, "Bad audio sample format");
            // get format information
            m_info = frame.info;
            m_sampleInfo = Audio::formatInfo(frame.info.sampleFormat);
            m_channelInfo = Audio::formatInfo(frame.info.channelFormat);
            // build RIFF / WAVE file header
            m_fileHeader.riffChunkSize = static_cast<uint32_t>(sizeof(WavHeader) - 8);
            m_fileHeader.dataFormat = m_sampleInfo.format == Audio::SampleFormat::Float32P ? 3 : 1; // IEEE 754 float (3) or integer PCM (1) data?
            m_fileHeader.nrOfChannels = m_channelInfo.nrOfChannels;
            m_fileHeader.sampleRateHz = m_info.sampleRateHz;
            m_fileHeader.bytesPerSec = ((m_sampleInfo.bitsPerSample + 7) / 8) * m_channelInfo.nrOfChannels * m_info.sampleRateHz;
            m_fileHeader.blockAlign = ((m_sampleInfo.bitsPerSample + 7) / 8) * m_channelInfo.nrOfChannels;
            m_fileHeader.bitsPerSample = m_sampleInfo.bitsPerSample;
            m_isFirstFrame = false;
        }
        else
        {
            REQUIRE(frame.info.channelFormat == m_info.channelFormat, std::runtime_error, "Frame audio channel format does not match");
            REQUIRE(frame.info.sampleRateHz == m_info.sampleRateHz, std::runtime_error, "Frame audio sample rate does not match");
            REQUIRE(frame.info.sampleFormat == m_info.sampleFormat, std::runtime_error, "Frame audio sample format does not match");
        }
        // get raw sample data
        REQUIRE(checkSampleFormat(frame.data, m_info.sampleFormat), std::runtime_error, "Sample data type does not match sample format " << m_sampleInfo.id);
        auto rawSampleData = AudioHelpers::toRawInterleavedData(frame.data, m_info.channelFormat);
        // RIFF / WAVE only supports 8-bit unsigned, 16-bit signed and 32-bit float PCM data. Other formats need to be converted
        if (m_info.sampleFormat == Audio::SampleFormat::Signed8P)
        {
            auto srcPtr = reinterpret_cast<const int8_t *>(rawSampleData.data());
            auto dstPtr = reinterpret_cast<uint8_t *>(rawSampleData.data());
            for (std::size_t i = 0; i < rawSampleData.size(); ++i)
            {
                auto v = srcPtr[i] < -127 ? -127 : srcPtr[i];
                dstPtr[i] = static_cast<int32_t>(v) + 128;
            }
        }
        else if (m_info.sampleFormat == Audio::SampleFormat::Unsigned16P)
        {
            auto srcPtr = reinterpret_cast<const uint16_t *>(rawSampleData.data());
            auto dstPtr = reinterpret_cast<int16_t *>(rawSampleData.data());
            for (std::size_t i = 0; i < rawSampleData.size() / 2; ++i)
            {
                auto v = srcPtr[i] < 1 ? 1 : srcPtr[i];
                dstPtr[i] = static_cast<int32_t>(srcPtr[i]) - 32768;
            }
        }
        // update file header
        m_fileHeader.riffChunkSize += static_cast<uint32_t>(rawSampleData.size());
        m_fileHeader.dataSize += static_cast<uint32_t>(rawSampleData.size());
        // write header to start of file
        m_os.seekp(0);
        REQUIRE(!m_os.fail(), std::runtime_error, "Failed to set write position to start of file");
        m_os.write(reinterpret_cast<const char *>(&m_fileHeader), sizeof(m_fileHeader));
        REQUIRE(!m_os.fail(), std::runtime_error, "Failed to write RIFF / WAVE header to file");
        // append sample data to end of file
        m_os.seekp(0, std::ios_base::end);
        REQUIRE(!m_os.fail(), std::runtime_error, "Failed to set write position to end of file");
        m_os.write(reinterpret_cast<const char *>(rawSampleData.data()), rawSampleData.size());
        REQUIRE(!m_os.fail(), std::runtime_error, "Failed to write audio sample data to file");
    }

    auto WavWriter::close() -> void
    {
        m_os.close();
        m_isFirstFrame = true;
    }

    WavWriter::~WavWriter()
    {
        close();
    }
}
