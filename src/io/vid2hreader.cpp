#include "vid2hreader.h"

#include "codec/dxtv.h"
#include "color/colorhelpers.h"
#include "compression/lzss.h"
#include "processing/processingtypes.h"

namespace Video
{

    auto Vid2hReader::open(const std::string &filePath) -> void
    {
        // open input file
        auto fileVid2h = std::ifstream(filePath, std::ios::in | std::ios::binary);
        REQUIRE(fileVid2h.is_open() && !m_is.fail(), std::runtime_error, "Failed to open " << filePath << " for reading");
        m_is = std::move(fileVid2h);
        // try reading video file header
        m_fileHeader = IO::Vid2h::readFileHeader(m_is);
        REQUIRE(m_fileHeader.width != 0 && m_fileHeader.height != 0, std::runtime_error, "Width or height can not be 0");
        REQUIRE(m_fileHeader.nrOfFrames != 0, std::runtime_error, "Number of frames can not be 0");
        REQUIRE(m_fileHeader.fps != 0, std::runtime_error, "Frames rate can not be 0");
        // generate video info
        m_info.codecName = "vid2h";
        m_info.videoStreamIndex = 0;
        m_info.width = m_fileHeader.width;
        m_info.height = m_fileHeader.height;
        m_info.fps = static_cast<double>(m_fileHeader.fps) / 32768.0;
        m_info.nrOfFrames = m_fileHeader.nrOfFrames;
        m_info.durationS = static_cast<double>(m_fileHeader.nrOfFrames) / m_info.fps;
        m_info.pixelFormat = Color::findFormat(m_fileHeader.bitsPerPixel, m_fileHeader.colorMapEntries != 0, m_fileHeader.swappedRedBlue);
        m_info.colorMapFormat = Color::findFormat(m_fileHeader.bitsPerColor, false, m_fileHeader.swappedRedBlue);
    }

    auto Vid2hReader::getInfo() const -> VideoInfo
    {
        return m_info;
    }

    auto Vid2hReader::readFrame() -> std::vector<Color::XRGB8888>
    {
        REQUIRE(m_is.is_open() && !m_is.fail(), std::runtime_error, "File stream not open");
        auto frame = IO::Vid2h::readFrame(m_is, m_fileHeader);
        auto pixelData = frame.pixelData;
        REQUIRE(!pixelData.empty(), std::runtime_error, "Frame pixel data empty");
        // split chunk data into header and data
        std::vector<Color::XRGB8888> frameData;
        do
        {
            const auto [srcHeader, srcData] = IO::Vid2h::splitChunk(pixelData);
            const auto isFinal = (srcHeader.processingType & Image::ProcessingTypeFinal) != 0;
            const auto processingType = static_cast<Image::ProcessingType>(srcHeader.processingType & (~Image::ProcessingTypeFinal));
            // reverse processing operation used in this stage
            switch (processingType)
            {
            case Image::ProcessingType::CompressLZ10:
                pixelData = Compression::decodeLZ10(srcData);
                REQUIRE(pixelData.size() == srcHeader.uncompressedSize, std::runtime_error, "Bad uncompressed size");
                break;
            case Image::ProcessingType::CompressDXTV:
                frameData = DXTV::decode(srcData, m_previousFrame, m_fileHeader.width, m_fileHeader.height, m_fileHeader.swappedRedBlue);
                REQUIRE(frameData.size() * sizeof(Color::XRGB8888) == srcHeader.uncompressedSize, std::runtime_error, "Bad uncompressed size");
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
        if (frameData.empty())
        {
            frameData = ColorHelpers::toXRGB8888(pixelData, m_info.pixelFormat, frame.colorMapData, m_info.colorMapFormat);
        }
        m_previousFrame = frameData;
        return frameData;
    }

    auto Vid2hReader::close() -> void
    {
        m_is.close();
    }
}