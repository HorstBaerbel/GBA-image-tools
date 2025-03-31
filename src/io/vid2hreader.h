#pragma once

#include "vid2hio.h"
#include "videoreader.h"

#include <cstdint>
#include <fstream>
#include <memory>
#include <string>
#include <vector>

namespace Video
{

    /// @brief Video reader class that reads our proprietary format and returns data in XRGB8888 format
    class Vid2hReader : public Reader
    {
    public:
        /// @brief Constructor
        Vid2hReader() = default;

        /// @brief Destruktor. Calls close()
        virtual ~Vid2hReader() = default;

        /// @brief Open reader on a file so you can later readFrame() from it
        /// @throw Throws a std::runtime_errror if anything goes wrong
        virtual auto open(const std::string &filePath) -> void override;

        /// @brief Get information about opened video file
        virtual auto getInfo() const -> VideoInfo override;

        /// @brief Read next XRGB8888 frame from video. Will return empty data if EOF
        virtual auto readFrame() -> std::vector<Color::XRGB8888> override;

        /// @brief Close reader opened with open()
        virtual auto close() -> void override;

    private:
        IO::Vid2h::FileHeader m_fileHeader;
        VideoInfo m_info;
        std::vector<Color::XRGB8888> m_previousFrame;
        std::ifstream m_is;
    };

}
