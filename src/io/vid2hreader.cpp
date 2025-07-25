#include "vid2hreader.h"

#include "audio/audiohelpers.h"
#include "audio/processingtype.h"
#include "codec/dxtv.h"
#include "color/colorhelpers.h"
#include "compression/lzss.h"
#include "image/processingtype.h"

namespace Media
{

    auto Vid2hReader::open(const std::string &filePath) -> void
    {
        // open input file
        auto fileVid2h = std::ifstream(filePath, std::ios::in | std::ios::binary);
        REQUIRE(fileVid2h.is_open() && !fileVid2h.fail(), std::runtime_error, "Failed to open " << filePath << " for reading");
        m_is = std::move(fileVid2h);
        // try reading video file header
        m_fileHeader = IO::Vid2h::readFileHeader(m_is);
        REQUIRE(m_fileHeader.contentType != IO::FileType::Unknown, std::runtime_error, "Bad file content type");
        m_info.fileType = m_fileHeader.contentType;
        // generate video info
        if ((m_fileHeader.contentType & IO::FileType::Video) != 0)
        {
            REQUIRE(m_fileHeader.videoNrOfFrames != 0, std::runtime_error, "Number of frames can not be 0");
            REQUIRE(m_fileHeader.videoFrameRateHz != 0, std::runtime_error, "Frame rate can not be 0");
            REQUIRE(m_fileHeader.videoWidth != 0 && m_fileHeader.videoHeight != 0, std::runtime_error, "Width or height can not be 0");
            REQUIRE(m_fileHeader.videoBitsPerPixel == 1 || m_fileHeader.videoBitsPerPixel == 2 || m_fileHeader.videoBitsPerPixel == 4 || m_fileHeader.videoBitsPerPixel == 8 || m_fileHeader.videoBitsPerPixel == 15 || m_fileHeader.videoBitsPerPixel == 16 || m_fileHeader.videoBitsPerPixel == 24, std::runtime_error, "Unsupported pixel bit depth: " << m_fileHeader.videoBitsPerPixel);
            REQUIRE(m_fileHeader.videoBitsPerColor == 0 || m_fileHeader.videoBitsPerColor == 15 || m_fileHeader.videoBitsPerColor == 16 || m_fileHeader.videoBitsPerPixel == 24, std::runtime_error, "Unsupported color map bit depth: " << m_fileHeader.videoBitsPerColor);
            m_info.videoNrOfFrames = m_fileHeader.videoNrOfFrames;
            const double fps = static_cast<double>(m_fileHeader.videoFrameRateHz) / 65536.0;
            m_info.videoFrameRateHz = fps;
            m_info.videoDurationS = static_cast<double>(m_fileHeader.videoNrOfFrames) / fps;
            m_info.videoCodecName = "vid2h";
            m_info.videoStreamIndex = 0;
            m_info.videoWidth = m_fileHeader.videoWidth;
            m_info.videoHeight = m_fileHeader.videoHeight;
            m_info.videoPixelFormat = Color::findFormat(m_fileHeader.videoBitsPerPixel, m_fileHeader.videoColorMapEntries != 0, m_fileHeader.videoSwappedRedBlue);
            m_info.videoColorMapFormat = Color::findFormat(m_fileHeader.videoBitsPerColor, false, m_fileHeader.videoSwappedRedBlue);
        }
        // generate audio info
        if ((m_fileHeader.contentType & IO::FileType::Audio) != 0)
        {
            m_info.audioNrOfSamples = m_fileHeader.audioNrOfSamples;
            m_info.audioDurationS = static_cast<double>(m_fileHeader.audioNrOfSamples) / static_cast<double>(m_fileHeader.audioSampleRateHz);
            m_info.audioCodecName = "vid2h";
            m_info.audioStreamIndex = 0;
            m_info.audioSampleRateHz = m_fileHeader.audioSampleRateHz;
            REQUIRE(m_fileHeader.audioChannels == 1 || m_fileHeader.audioChannels == 2, std::runtime_error, "Number of audio channels must 1 or 2");
            m_info.audioChannelFormat = m_fileHeader.audioChannels == 1 ? Audio::ChannelFormat::Mono : Audio::ChannelFormat::Stereo;
            REQUIRE(m_fileHeader.audioSampleBits == 8 || m_fileHeader.audioSampleBits == 16 || m_fileHeader.audioSampleBits == 32, std::runtime_error, "Number of audio samples must 8, 16 or or 32");
            m_info.audioSampleFormat = Audio::findSampleFormat(m_fileHeader.audioSampleBits, true);
            m_info.audioOffsetS = static_cast<double>(m_fileHeader.audioOffsetSamples) / static_cast<double>(m_fileHeader.audioSampleRateHz);
        }
    }

    auto Vid2hReader::getInfo() const -> MediaInfo
    {
        return m_info;
    }

    auto Vid2hReader::readFrame() -> FrameData
    {
        REQUIRE(m_is.is_open() && !m_is.fail(), std::runtime_error, "File stream not open");
        auto frame = IO::Vid2h::readFrame(m_is, m_fileHeader);
        if (frame.first == IO::FrameType::Colormap)
        {
            // convert color map to XRGB8888
            REQUIRE(!frame.second.empty(), std::runtime_error, "Frame color map data empty");
            REQUIRE(m_info.videoColorMapFormat != Color::Format::Unknown, std::runtime_error, "Bad color map format");
            std::vector<Color::XRGB8888> outColorMap;
            outColorMap = ColorHelpers::toXRGB8888(frame.second, m_info.videoColorMapFormat);
            m_previousColorMap = outColorMap;
            return {IO::FrameType::Colormap, outColorMap};
        }
        else if (frame.first == IO::FrameType::Pixels)
        {
            auto inData = frame.second;
            REQUIRE(!inData.empty(), std::runtime_error, "Frame pixel data empty");
            // split chunk data into header and data
            std::vector<Color::XRGB8888> outData;
            do
            {
                const auto [srcHeader, srcData] = IO::Vid2h::splitChunk(inData);
                const auto isFinal = (srcHeader.processingType & Image::ProcessingTypeFinal) != 0;
                const auto processingType = static_cast<Image::ProcessingType>(srcHeader.processingType & (~Image::ProcessingTypeFinal));
                // reverse processing operation used in this stage
                switch (processingType)
                {
                case Image::ProcessingType::CompressLZ10:
                    inData = Compression::decodeLZ10(srcData);
                    REQUIRE(inData.size() == srcHeader.uncompressedSize, std::runtime_error, "Bad uncompressed size");
                    break;
                case Image::ProcessingType::CompressDXTV:
                    outData = DXTV::decode(srcData, m_previousPixels, m_fileHeader.videoWidth, m_fileHeader.videoHeight, m_fileHeader.videoSwappedRedBlue);
                    REQUIRE(outData.size() * sizeof(Color::XRGB8888) == srcHeader.uncompressedSize, std::runtime_error, "Bad uncompressed size");
                    break;
                default:
                    THROW(std::runtime_error, "Unsupported processing type " << static_cast<uint32_t>(processingType));
                }
                // break if this was the last processing operation
                if (isFinal)
                {
                    break;
                }
            } while (true);
            // return color data or convert pixel data to XRGB8888
            if (outData.empty())
            {
                outData = ColorHelpers::toXRGB8888(inData, m_info.videoPixelFormat, m_previousColorMap);
            }
            m_previousPixels = outData;
            return {IO::FrameType::Pixels, outData};
        }
        else
        {
            auto inData = frame.second;
            REQUIRE(!inData.empty(), std::runtime_error, "Frame audio data empty");
            // split chunk data into header and data
            std::vector<int16_t> outData;
            do
            {
                const auto [srcHeader, srcData] = IO::Vid2h::splitChunk(inData);
                const auto isFinal = (srcHeader.processingType & Audio::ProcessingTypeFinal) != 0;
                const auto processingType = static_cast<Audio::ProcessingType>(srcHeader.processingType & (~Audio::ProcessingTypeFinal));
                // reverse processing operation used in this stage
                switch (processingType)
                {
                case Audio::ProcessingType::Uncompressed:
                    outData = AudioHelpers::toSigned16(inData, m_info.audioSampleFormat);
                    break;
                case Audio::ProcessingType::CompressLZ10:
                    inData = Compression::decodeLZ10(srcData);
                    REQUIRE(inData.size() == srcHeader.uncompressedSize, std::runtime_error, "Bad uncompressed size");
                    break;
                default:
                    THROW(std::runtime_error, "Unsupported processing type " << static_cast<uint32_t>(processingType));
                }
                // break if this was the last processing operation
                if (isFinal)
                {
                    break;
                }
            } while (true);
            // clamp audio data to [-32767,32767] instead of [-32768,32767] to center around 0
            std::for_each(outData.begin(), outData.end(), [](auto &v)
                          { v = v < -32767 ? -32767 : v; });
            return {IO::FrameType::Audio, outData};
        }
    }

    auto Vid2hReader::close() -> void
    {
        m_is.close();
    }
}
