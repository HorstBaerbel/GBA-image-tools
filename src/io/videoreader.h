#pragma once

#include <cstdint>
#include <string>
#include <vector>

/// @brief Video reader interface
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
        double fps = 0;
        uint64_t nrOfFrames = 0;
        double durationS = 0;
    };

    /// @brief Destruktor. Calls close()
    virtual ~VideoReader();

    /// @brief Open reader on a file so you can later readFrame() from it
    /// @throw Throws a std::runtime_errror if anything goes wrong
    virtual auto open(const std::string &filePath) -> void = 0;

    /// @brief Get information about opened video file
    virtual auto getInfo() const -> VideoInfo = 0;

    /// @brief Read next XRGB8888 frame from video. Will return empty data if EOF
    virtual auto readFrame() const -> std::vector<uint32_t> = 0;

    /// @brief Close reader opened with open()
    virtual auto close() -> void;
};
