#pragma once

#include "videoreader.h"

#include <cstdint>
#include <memory>
#include <string>
#include <vector>

namespace Video
{

    /// @brief Video reader class that uses FFmpeg and returns data in XRGB8888 format
    class FFmpegReader : public Reader
    {
    public:
        /// @brief Constructor
        FFmpegReader();

        /// @brief Destruktor. Calls close()
        virtual ~FFmpegReader();

        /// @brief Open FFmpeg reader on a file so you can later readFrame() from it
        /// @throw Throws a std::runtime_errror if anything goes wrong
        virtual auto open(const std::string &filePath) -> void override;

        /// @brief Get information about opened video file
        virtual auto getInfo() const -> VideoInfo override;

        /// @brief Read next XRGB8888 frame from video. Will return empty data if EOF
        virtual auto readFrame() const -> std::vector<uint32_t> override;

        /// @brief Close FFmpeg reader opened with open()
        virtual auto close() -> void override;

    private:
        struct ReaderState;
        std::shared_ptr<ReaderState> m_state;
    };
}
