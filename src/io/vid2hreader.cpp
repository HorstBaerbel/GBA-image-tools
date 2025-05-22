#include "vid2hreader.h"

#include "codec/dxtv.h"
#include "color/colorhelpers.h"
#include "compression/lzss.h"
#include "processing/processingtypes.h"

namespace Media
{

    auto Vid2hReader::open(const std::string &filePath) -> void
    {
        // open input file
        auto fileVid2h = std::ifstream(filePath, std::ios::in | std::ios::binary);
        REQUIRE(fileVid2h.is_open() && !m_is.fail(), std::runtime_error, "Failed to open " << filePath << " for reading");
        m_is = std::move(fileVid2h);
        // try reading video file header
        m_fileHeader = IO::Vid2h::readFileHeader(m_is);
        REQUIRE(m_fileHeader.nrOfFrames != 0, std::runtime_error, "Number of frames can not be 0");
        REQUIRE(m_fileHeader.videoWidth != 0 && m_fileHeader.videoHeight != 0, std::runtime_error, "Width or height can not be 0");
        REQUIRE(m_fileHeader.videoFps != 0, std::runtime_error, "Frames rate can not be 0");
        REQUIRE(m_fileHeader.videoBitsPerColor == 0 || m_fileHeader.videoBitsPerColor == 15 || m_fileHeader.videoBitsPerColor == 16, std::runtime_error, "Unsupported color map bit depth: " << m_fileHeader.videoBitsPerColor);
        // generate video info
        m_info.videoCodecName = "vid2h";
        m_info.durationS = static_cast<double>(m_fileHeader.nrOfFrames) / m_info.videoFps;
        m_info.videoStreamIndex = 0;
        m_info.videoWidth = m_fileHeader.videoWidth;
        m_info.videoHeight = m_fileHeader.videoHeight;
        m_info.videoFps = static_cast<double>(m_fileHeader.videoFps) / 65536.0;
        m_info.videoNrOfFrames = m_fileHeader.nrOfFrames;
        m_info.videoPixelFormat = Color::findFormat(m_fileHeader.videoBitsPerPixel, m_fileHeader.videoColorMapEntries != 0, m_fileHeader.videoSwappedRedBlue);
        m_info.videoColorMapFormat = Color::findFormat(m_fileHeader.videoBitsPerColor, false, m_fileHeader.videoSwappedRedBlue);
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
            REQUIRE(!frame.second.empty(), std::runtime_error, "Color map data empty");
            REQUIRE(m_fileHeader.videoBitsPerColor == 15 || m_fileHeader.videoBitsPerColor == 16, std::runtime_error, "Unsupported color map bit depth: " << m_fileHeader.videoBitsPerColor);
            m_previousColorMap = frame.second;
            return {IO::FrameType::Colormap, frame.second};
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
                outData = ColorHelpers::toXRGB8888(inData, m_info.videoPixelFormat, m_previousColorMap, m_info.videoColorMapFormat);
            }
            m_previousPixels = outData;
            return {IO::FrameType::Pixels, outData};
        }
        else
        {
            return {IO::FrameType::Audio, {}};
        }
    }

    auto Vid2hReader::close() -> void
    {
        m_is.close();
    }
}
