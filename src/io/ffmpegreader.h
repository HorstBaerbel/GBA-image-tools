#pragma once

#include <cstdint>
#include <memory>
#include <string>
#include <vector>

/// @brief Video reader class that uses FFmpeg and returns data in XRGB8888 format
class FFmpegReader
{
public:
    /// @brief Video information about opened video file
    struct VideoInfo
    {
        std::string codecName;
        uint32_t videoStreamIndex = 0;
        uint32_t width = 0;
        uint32_t height = 0;
        double fps = 0;
        uint64_t nrOfFrames = 0;
        double durationS = 0;
    };

    /// @brief Constructor
    FFmpegReader();

    /// @brief Destruktor. Calls close()
    ~FFmpegReader();

    /// @brief Open FFmpeg reader on a file so you can later readFrame() from it
    /// @throw Throws a std::runtime_errror if anything goes wrong
    auto open(const std::string &filePath) -> void;

    /// @brief Get information about opened video file
    auto getInfo() const -> VideoInfo;

    /// @brief Read next XRGB8888 frame from video. Will return empty data if EOF
    auto readFrame() const -> std::vector<uint32_t>;

    /// @brief Open FFmpeg reader opened with open()
    auto close() -> void;

private:
    struct ReaderState;
    std::shared_ptr<ReaderState> m_state;
};
