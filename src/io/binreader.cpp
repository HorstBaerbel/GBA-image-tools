#include "binreader.h"

namespace Video
{

    auto BinReader::open(const std::string &filePath) -> void
    {
        // open input file
        auto fileStream = std::ifstream(filePath, std::ios::in | std::ios::binary);
        REQUIRE(fileStream.is_open() && !m_is.fail(), std::runtime_error, "Failed to open " << filePath << " for reading");
        m_is = std::move(fileStream);
        // try reading video info
        m_fileHeader = IO::Stream::readFileHeader(m_is);
        REQUIRE(m_fileHeader.width != 0 && m_fileHeader.height != 0, std::runtime_error, "Width or height can not be 0");
        REQUIRE(m_fileHeader.nrOfFrames != 0, std::runtime_error, "Number of frames can not be 0");
        REQUIRE(m_fileHeader.fps != 0, std::runtime_error, "Frames rate can not be 0");
    }

    auto BinReader::getInfo() const -> VideoInfo
    {
        VideoInfo info;
        info.codecName = "vid2h";
        info.videoStreamIndex = 0;
        info.width = m_fileHeader.width;
        info.height = m_fileHeader.height;
        info.fps = m_fileHeader.fps;
        info.nrOfFrames = m_fileHeader.nrOfFrames;
        info.durationS = static_cast<double>(m_fileHeader.nrOfFrames) / m_fileHeader.fps;
        return info;
    }

    auto BinReader::readFrame() -> std::vector<uint32_t>
    {
        REQUIRE(m_is.is_open() && !m_is.fail(), std::runtime_error, "File stream not open");
        auto data = IO::Stream::readFrame(m_is, m_fileHeader);
        return {};
    }

    auto BinReader::close() -> void
    {
        m_is.close();
    }
}