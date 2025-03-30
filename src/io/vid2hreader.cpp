#include "vid2hreader.h"

namespace Video
{

    auto Vid2hReader::open(const std::string &filePath) -> void
    {
        // open input file
        auto fileVid2h = std::ifstream(filePath, std::ios::in | std::ios::binary);
        REQUIRE(fileVid2h.is_open() && !m_is.fail(), std::runtime_error, "Failed to open " << filePath << " for reading");
        m_is = std::move(fileVid2h);
        // try reading video info
        m_fileHeader = IO::Vid2h::readFileHeader(m_is);
        REQUIRE(m_fileHeader.width != 0 && m_fileHeader.height != 0, std::runtime_error, "Width or height can not be 0");
        REQUIRE(m_fileHeader.nrOfFrames != 0, std::runtime_error, "Number of frames can not be 0");
        REQUIRE(m_fileHeader.fps != 0, std::runtime_error, "Frames rate can not be 0");
    }

    auto Vid2hReader::getInfo() const -> VideoInfo
    {
        VideoInfo info;
        info.codecName = "vid2h";
        info.videoStreamIndex = 0;
        info.width = m_fileHeader.width;
        info.height = m_fileHeader.height;
        info.fps = static_cast<double>(m_fileHeader.fps) / 32768.0;
        info.nrOfFrames = m_fileHeader.nrOfFrames;
        info.durationS = static_cast<double>(m_fileHeader.nrOfFrames) / m_fileHeader.fps;
        return info;
    }

    auto Vid2hReader::readFrame() -> std::vector<Color::XRGB8888>
    {
        REQUIRE(m_is.is_open() && !m_is.fail(), std::runtime_error, "File stream not open");
        auto data = IO::Vid2h::readFrame(m_is, m_fileHeader);
        REQUIRE(!data.pixelData.empty(), std::runtime_error, "Frame pixel data empty");
        // decode pixel data dependening on chunk header

        return {};
    }

    auto Vid2hReader::close() -> void
    {
        m_is.close();
    }
}