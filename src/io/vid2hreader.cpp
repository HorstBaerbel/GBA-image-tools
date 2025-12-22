#include "vid2hreader.h"

#include "audio/audiohelpers.h"
#include "audio_codec/adpcm.h"
#include "color/colorhelpers.h"
#include "compression/lz4.h"
#include "compression/lzss.h"
#include "if/audio_processingtype.h"
#include "if/image_processingtype.h"
#include "video_codec/dxtv.h"

namespace Media
{

    auto Vid2hReader::open(const std::string &filePath) -> void
    {
        // open input file
        auto fileVid2h = std::ifstream(filePath, std::ios::in | std::ios::binary);
        REQUIRE(fileVid2h.is_open() && !fileVid2h.fail(), std::runtime_error, "Failed to open " << filePath << " for reading");
        m_is = std::move(fileVid2h);
        // try reading video file header
        m_fileDataInfo = IO::Vid2h::readFileHeader(m_is);
        REQUIRE(m_fileDataInfo.contentType != IO::FileType::Unknown, std::runtime_error, "Bad file content type");
        m_info.fileType = m_fileDataInfo.contentType;
        // read audio info
        if (m_fileDataInfo.contentType & IO::FileType::Audio)
        {
            m_audioHeader = IO::Vid2h::readAudioHeader(m_is, m_fileDataInfo);
            REQUIRE(m_audioHeader.nrOfFrames != 0, std::runtime_error, "Number of audio frames can not be 0");
            m_info.audioNrOfFrames = m_audioHeader.nrOfFrames;
            m_info.audioNrOfSamples = m_audioHeader.nrOfSamples;
            m_info.audioDurationS = static_cast<double>(m_audioHeader.nrOfSamples) / static_cast<double>(m_audioHeader.sampleRateHz);
            m_info.audioCodecName = "vid2h (";
            for (uint32_t pi = 0; pi < sizeof(m_audioHeader.processing); ++pi)
            {
                const auto processing = m_audioHeader.processing[pi];
                if (processing == Audio::ProcessingType::Invalid)
                {
                    break;
                }
                m_info.audioCodecName += std::to_string(static_cast<int>(processing));
                if (pi < 3 && m_audioHeader.processing[pi + 1] != Audio::ProcessingType::Invalid)
                {
                    m_info.audioCodecName += ", ";
                }
            }
            m_info.audioCodecName += ")";
            m_info.audioStreamIndex = 0;
            m_info.audioSampleRateHz = m_audioHeader.sampleRateHz;
            REQUIRE(m_audioHeader.channels == 1 || m_audioHeader.channels == 2, std::runtime_error, "Number of audio channels must 1 or 2");
            m_info.audioChannelFormat = m_audioHeader.channels == 1 ? Audio::ChannelFormat::Mono : Audio::ChannelFormat::Stereo;
            REQUIRE(m_audioHeader.sampleBits == 8 || m_audioHeader.sampleBits == 16 || m_audioHeader.sampleBits == 32, std::runtime_error, "Number of audio samples must 8, 16 or or 32");
            m_info.audioSampleFormat = Audio::findSampleFormat(m_audioHeader.sampleBits, true);
            m_info.audioOffsetS = static_cast<double>(m_audioHeader.offsetSamples) / static_cast<double>(m_audioHeader.sampleRateHz);
        }
        // read video info
        if (m_fileDataInfo.contentType & IO::FileType::Video)
        {
            m_videoHeader = IO::Vid2h::readVideoHeader(m_is, m_fileDataInfo);
            REQUIRE(m_videoHeader.nrOfFrames != 0, std::runtime_error, "Number of video frames can not be 0");
            REQUIRE(m_videoHeader.frameRateHz != 0, std::runtime_error, "Frame rate can not be 0");
            REQUIRE(m_videoHeader.width != 0 && m_videoHeader.height != 0, std::runtime_error, "Width or height can not be 0");
            REQUIRE(m_videoHeader.bitsPerPixel == 1 || m_videoHeader.bitsPerPixel == 2 || m_videoHeader.bitsPerPixel == 4 || m_videoHeader.bitsPerPixel == 8 || m_videoHeader.bitsPerPixel == 15 || m_videoHeader.bitsPerPixel == 16 || m_videoHeader.bitsPerPixel == 24, std::runtime_error, "Unsupported pixel bit depth: " << m_videoHeader.bitsPerPixel);
            REQUIRE(m_videoHeader.bitsPerColor == 0 || m_videoHeader.bitsPerColor == 15 || m_videoHeader.bitsPerColor == 16 || m_videoHeader.bitsPerPixel == 24, std::runtime_error, "Unsupported color map bit depth: " << m_videoHeader.bitsPerColor);
            REQUIRE(m_videoHeader.bitsPerColor == 0 || m_videoHeader.nrOfColorMapFrames != 0, std::runtime_error, "Color map format specified, but number of color map frames is 0");
            m_info.videoNrOfFrames = m_videoHeader.nrOfFrames;
            const double fps = static_cast<double>(m_videoHeader.frameRateHz) / 65536.0;
            m_info.videoFrameRateHz = fps;
            m_info.videoDurationS = static_cast<double>(m_videoHeader.nrOfFrames) / fps;
            m_info.videoCodecName = "vid2h (";
            for (uint32_t pi = 0; pi < sizeof(m_videoHeader.processing); ++pi)
            {
                const auto processing = m_videoHeader.processing[pi];
                if (processing == Image::ProcessingType::Invalid)
                {
                    break;
                }
                m_info.videoCodecName += std::to_string(static_cast<int>(processing));
                if (pi < 3 && m_videoHeader.processing[pi + 1] != Image::ProcessingType::Invalid)
                {
                    m_info.videoCodecName += ", ";
                }
            }
            m_info.videoCodecName += ")";
            m_info.videoStreamIndex = 0;
            m_info.videoWidth = m_videoHeader.width;
            m_info.videoHeight = m_videoHeader.height;
            m_info.videoPixelFormat = Color::findFormat(m_videoHeader.bitsPerPixel, m_videoHeader.colorMapEntries != 0, m_videoHeader.swappedRedBlue);
            m_info.videoColorMapFormat = Color::findFormat(m_videoHeader.bitsPerColor, false, m_videoHeader.swappedRedBlue);
        }
        if (m_fileDataInfo.contentType & IO::FileType::Subtitles)
        {
            m_subtitlesHeader = IO::Vid2h::readSubtitlesHeader(m_is, m_fileDataInfo);
            REQUIRE(m_subtitlesHeader.nrOfFrames != 0, std::runtime_error, "Number of subtitles frames can not be 0");
            m_info.subtitlesNrOfFrames = m_subtitlesHeader.nrOfFrames;
        }
        // read meta data if any
        if (m_fileDataInfo.metaDataOffset > 0)
        {
            m_metaData = IO::Vid2h::readMetaData(m_is, m_fileDataInfo);
            m_info.metaDataSize = m_metaData.size();
        }
        m_is.seekg(m_fileDataInfo.frameDataOffset);
    }

    auto Vid2hReader::getInfo() const -> MediaInfo
    {
        return m_info;
    }

    auto Vid2hReader::getMetaData() const -> std::vector<uint8_t>
    {
        return m_metaData;
    }

    auto Vid2hReader::readFrame() -> FrameData
    {
        REQUIRE(m_is.is_open() && !m_is.fail(), std::runtime_error, "File stream not open");
        auto frame = IO::Vid2h::readFrame(m_is);
        if (frame.first == IO::FrameType::Pixels)
        {
            auto inData = frame.second;
            REQUIRE(!inData.empty(), std::runtime_error, "Frame pixel data empty");
            std::vector<Color::XRGB8888> outData;
            // do decoding steps
            for (uint32_t pi = 0; pi < sizeof(m_videoHeader.processing); ++pi)
            {
                // this is the final operation either if we don't have any more steps, the current step is just a copy, or the next step is invalid
                const auto processingType = m_videoHeader.processing[pi] == Image::ProcessingType::Invalid ? Image::ProcessingType::Uncompressed : m_videoHeader.processing[pi];
                const auto isFinal = (pi >= 3) || (processingType == Image::ProcessingType::Uncompressed) || (m_videoHeader.processing[pi + 1] == Image::ProcessingType::Invalid);
                // reverse processing operation used in this stage
                switch (processingType)
                {
                case Image::ProcessingType::CompressLZ4_40:
                    inData = Compression::decodeLZ4_40(inData);
                    break;
                case Image::ProcessingType::CompressLZSS_10:
                    inData = Compression::decodeLZSS_10(inData);
                    break;
                case Image::ProcessingType::CompressDXTV:
                    outData = Video::Dxtv::decode(inData, m_previousPixels, m_videoHeader.width, m_videoHeader.height, m_videoHeader.swappedRedBlue);
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
            return {IO::FrameType::Pixels, 0.0, outData};
        }
        else if (frame.first == IO::FrameType::Colormap)
        {
            // convert color map to XRGB8888
            REQUIRE(!frame.second.empty(), std::runtime_error, "Frame color map data empty");
            REQUIRE(m_info.videoColorMapFormat != Color::Format::Unknown, std::runtime_error, "Bad color map format");
            std::vector<Color::XRGB8888> outColorMap;
            outColorMap = ColorHelpers::toXRGB8888(frame.second, m_info.videoColorMapFormat);
            m_previousColorMap = outColorMap;
            return {IO::FrameType::Colormap, 0.0, outColorMap};
        }
        else if (frame.first == IO::FrameType::Audio)
        {
            auto inData = frame.second;
            REQUIRE(!inData.empty(), std::runtime_error, "Frame audio data empty");
            std::vector<int16_t> outData;
            // do decoding steps
            for (uint32_t pi = 0; pi < sizeof(m_audioHeader.processing); ++pi)
            {
                // this is the final operation either if we don't have any more steps, the current step is just a copy, or the next step is invalid
                const auto processingType = m_audioHeader.processing[pi] == Audio::ProcessingType::Invalid ? Audio::ProcessingType::Uncompressed : m_audioHeader.processing[pi];
                const auto isFinal = (pi >= 3) || (processingType == Audio::ProcessingType::Uncompressed) || (m_audioHeader.processing[pi + 1] == Audio::ProcessingType::Invalid);
                // reverse processing operation used in this stage
                switch (processingType)
                {
                case Audio::ProcessingType::Uncompressed:
                    outData = AudioHelpers::toSigned16(inData, m_info.audioSampleFormat);
                    break;
                case Audio::ProcessingType::CompressLZ4_40:
                    inData = Compression::decodeLZ4_40(inData);
                    break;
                case Audio::ProcessingType::CompressLZSS_10:
                    inData = Compression::decodeLZSS_10(inData);
                    break;
                case Audio::ProcessingType::CompressADPCM:
                    outData = std::get<std::vector<int16_t>>(Audio::Adpcm::decode(inData));
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
            return {IO::FrameType::Audio, 0.0, outData};
        }
        else if (frame.first == IO::FrameType::Subtitles)
        {
            auto inData = frame.second;
            REQUIRE(inData.size() > (4 + 4 + 1), std::runtime_error, "Subtitles frame data too small");
            REQUIRE(inData.size() <= (4 + 4 + 1 + Subtitles::MaxSubTitleLength), std::runtime_error, "Subtitles frame data too big");
            Subtitles::RawData outData;
            outData.startTimeS = static_cast<double>(*reinterpret_cast<int32_t *>(inData.data() + 0)) / 65536.0;
            outData.endTimeS = static_cast<double>(*reinterpret_cast<int32_t *>(inData.data() + 4)) / 65536.0;
            outData.text = reinterpret_cast<const char *>(inData.data() + 8);
            REQUIRE(outData.text.size() > 0 && outData.text.size() <= (inData.size() - 8) && outData.text.size() <= Subtitles::MaxSubTitleLength, std::runtime_error, "Bad subtitles string size");
            return {IO::FrameType::Subtitles, outData.startTimeS, outData};
        }
        return {IO::FrameType::Unknown, 0.0, {}};
    }

    auto Vid2hReader::close() -> void
    {
        m_is.close();
    }
}
