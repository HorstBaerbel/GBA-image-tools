#include "vid2hreader.h"

#include "audio/audiohelpers.h"
#include "audio/processingtype.h"
#include "audio_codec/adpcm.h"
#include "color/colorhelpers.h"
#include "compression/lzss.h"
#include "image/processingtype.h"
#include "image_codec/dxtv.h"

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
            REQUIRE(m_fileHeader.video.nrOfFrames != 0, std::runtime_error, "Number of frames can not be 0");
            REQUIRE(m_fileHeader.video.frameRateHz != 0, std::runtime_error, "Frame rate can not be 0");
            REQUIRE(m_fileHeader.video.width != 0 && m_fileHeader.video.height != 0, std::runtime_error, "Width or height can not be 0");
            REQUIRE(m_fileHeader.video.bitsPerPixel == 1 || m_fileHeader.video.bitsPerPixel == 2 || m_fileHeader.video.bitsPerPixel == 4 || m_fileHeader.video.bitsPerPixel == 8 || m_fileHeader.video.bitsPerPixel == 15 || m_fileHeader.video.bitsPerPixel == 16 || m_fileHeader.video.bitsPerPixel == 24, std::runtime_error, "Unsupported pixel bit depth: " << m_fileHeader.video.bitsPerPixel);
            REQUIRE(m_fileHeader.video.bitsPerColor == 0 || m_fileHeader.video.bitsPerColor == 15 || m_fileHeader.video.bitsPerColor == 16 || m_fileHeader.video.bitsPerPixel == 24, std::runtime_error, "Unsupported color map bit depth: " << m_fileHeader.video.bitsPerColor);
            REQUIRE(m_fileHeader.video.bitsPerColor == 0 || m_fileHeader.video.nrOfColorMapFrames != 0, std::runtime_error, "Color map format specified, but number of color map frames is 0");
            m_info.videoNrOfFrames = m_fileHeader.video.nrOfFrames;
            const double fps = static_cast<double>(m_fileHeader.video.frameRateHz) / 65536.0;
            m_info.videoFrameRateHz = fps;
            m_info.videoDurationS = static_cast<double>(m_fileHeader.video.nrOfFrames) / fps;
            m_info.videoCodecName = "vid2h";
            m_info.videoStreamIndex = 0;
            m_info.videoWidth = m_fileHeader.video.width;
            m_info.videoHeight = m_fileHeader.video.height;
            m_info.videoPixelFormat = Color::findFormat(m_fileHeader.video.bitsPerPixel, m_fileHeader.video.colorMapEntries != 0, m_fileHeader.video.swappedRedBlue);
            m_info.videoColorMapFormat = Color::findFormat(m_fileHeader.video.bitsPerColor, false, m_fileHeader.video.swappedRedBlue);
        }
        // generate audio info
        if ((m_fileHeader.contentType & IO::FileType::Audio) != 0)
        {
            m_info.audioNrOfFrames = m_fileHeader.audio.nrOfFrames;
            m_info.audioNrOfSamples = m_fileHeader.audio.nrOfSamples;
            m_info.audioDurationS = static_cast<double>(m_fileHeader.audio.nrOfSamples) / static_cast<double>(m_fileHeader.audio.sampleRateHz);
            m_info.audioCodecName = "vid2h";
            m_info.audioStreamIndex = 0;
            m_info.audioSampleRateHz = m_fileHeader.audio.sampleRateHz;
            REQUIRE(m_fileHeader.audio.channels == 1 || m_fileHeader.audio.channels == 2, std::runtime_error, "Number of audio channels must 1 or 2");
            m_info.audioChannelFormat = m_fileHeader.audio.channels == 1 ? Audio::ChannelFormat::Mono : Audio::ChannelFormat::Stereo;
            REQUIRE(m_fileHeader.audio.sampleBits == 8 || m_fileHeader.audio.sampleBits == 16 || m_fileHeader.audio.sampleBits == 32, std::runtime_error, "Number of audio samples must 8, 16 or or 32");
            m_info.audioSampleFormat = Audio::findSampleFormat(m_fileHeader.audio.sampleBits, true);
            m_info.audioOffsetS = static_cast<double>(m_fileHeader.audio.offsetSamples) / static_cast<double>(m_fileHeader.audio.sampleRateHz);
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
            std::vector<Color::XRGB8888> outData;
            // do decoding steps
            for (uint32_t pi = 0; pi < sizeof(m_fileHeader.video.processing); ++pi)
            {
                // this is the final operation either if we don't have any more steps, the current step is just a copy, or the next step is invalid
                const auto processingType = m_fileHeader.video.processing[pi] == Image::ProcessingType::Invalid ? Image::ProcessingType::Uncompressed : m_fileHeader.video.processing[pi];
                const auto isFinal = (pi >= 3) || (processingType == Image::ProcessingType::Uncompressed) || (m_fileHeader.video.processing[pi + 1] == Image::ProcessingType::Invalid);
                // reverse processing operation used in this stage
                switch (processingType)
                {
                case Image::ProcessingType::CompressLZ10:
                    inData = Compression::decodeLZ10(inData);
                    break;
                case Image::ProcessingType::CompressDXTV:
                    outData = DXTV::decode(inData, m_previousPixels, m_fileHeader.video.width, m_fileHeader.video.height, m_fileHeader.video.swappedRedBlue);
                    break;
                default:
                    THROW(std::runtime_error, "Unsupported processing type " << static_cast<uint32_t>(processingType));
                }
                // break if this was the last processing operation
                if (isFinal)
                {
                    break;
                }
            }
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
            std::vector<int16_t> outData;
            // do decoding steps
            for (uint32_t pi = 0; pi < sizeof(m_fileHeader.audio.processing); ++pi)
            {
                // this is the final operation either if we don't have any more steps, the current step is just a copy, or the next step is invalid
                const auto processingType = m_fileHeader.audio.processing[pi] == Audio::ProcessingType::Invalid ? Audio::ProcessingType::Uncompressed : m_fileHeader.audio.processing[pi];
                const auto isFinal = (pi >= 3) || (processingType == Audio::ProcessingType::Uncompressed) || (m_fileHeader.audio.processing[pi + 1] == Audio::ProcessingType::Invalid);
                // reverse processing operation used in this stage
                switch (processingType)
                {
                case Audio::ProcessingType::Uncompressed:
                    outData = AudioHelpers::toSigned16(inData, m_info.audioSampleFormat);
                    break;
                case Audio::ProcessingType::CompressLZ10:
                    inData = Compression::decodeLZ10(inData);
                    break;
                case Audio::ProcessingType::CompressADPCM:
                    outData = std::get<std::vector<int16_t>>(Audio::ADPCM::decode(inData));
                    break;
                default:
                    THROW(std::runtime_error, "Unsupported processing type " << static_cast<uint32_t>(processingType));
                }
                // break if this was the last processing operation
                if (isFinal)
                {
                    break;
                }
            }
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
