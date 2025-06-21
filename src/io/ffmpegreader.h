#pragma once

#include "mediareader.h"

#include <cstdint>
#include <memory>
#include <string>
#include <vector>

namespace Media
{

    /// @brief Media reader class that uses FFmpeg and returns raw video and audio data
    class FFmpegReader : public Reader
    {
    public:
        /// @brief Constructor
        FFmpegReader();

        /// @brief Destruktor. Calls close()
        virtual ~FFmpegReader();

        /// @brief Open FFmpeg reader on a file so you can later readFrame() from it
        /// @throw Throws a std::runtime_error if anything goes wrong
        virtual auto open(const std::string &filePath) -> void override;

        /// @brief Get information about opened media file
        virtual auto getInfo() const -> MediaInfo override;

        /// @brief Read next video or audio frame. Will return FrameType::Unknown and empty data if EOF
        /// @note Pixel data will be returned as XRGB8888. Audio data will be returned as signed 16-bit samples. Multi-channel audio will be converted to stereo.
        virtual auto readFrame() -> FrameData override;

        /// @brief Close FFmpeg reader opened with open()
        virtual auto close() -> void override;

    private:
        struct State;
        std::shared_ptr<State> m_state;
        MediaInfo m_info;
    };
}
