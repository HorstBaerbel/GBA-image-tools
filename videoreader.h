#pragma once

#include <cstdint>
#include <memory>
#include <string>
#include <vector>

/// @brief Video reader class that uses FFmpeg and returns data in RGB888 format
class VideoReader
{
public:
    /// @brief Video information about opened video file
    struct VideoInfo
    {
        std::string codecName;
        uint32_t videoStreamIndex = 0;
        uint32_t width = 0;
        uint32_t height = 0;
        float fps = 0;
        uint64_t nrOfFrames = 0;
        float durationS = 0;
    };

    /// @brief Constructor
    VideoReader();

    /// @brief Destruktor. Calls close()
    ~VideoReader();

    /// @brief Open FFmpeg reader on a file so you can later getFrame() from it
    /// @throw Throws a std::runtime_errror if anything goes wrong
    void open(const std::string &filePath);

    /// @brief Get information about opened video file
    VideoInfo getInfo() const;

    /// @brief Read next RGB888 frame from video. Will return empty data if EOF
    std::vector<uint8_t> readFrame() const;

    /// @brief Open FFmpeg reader opened with open()
    void close();

private:
    struct ReaderState;
    std::shared_ptr<ReaderState> m_state;
};
