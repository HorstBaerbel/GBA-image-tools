#pragma once

#include "streamio.h"
#include "videoreader.h"

#include <cstdint>
#include <fstream>
#include <memory>
#include <string>
#include <vector>

namespace Video
{

    /// @brief Video reader class that reads our proprietary format and returns data in XRGB8888 format
    class BinReader : public Reader
    {
    public:
        /// @brief Constructor
        BinReader();

        /// @brief Destruktor. Calls close()
        virtual ~BinReader() = default;

        /// @brief Open reader on a file so you can later readFrame() from it
        /// @throw Throws a std::runtime_errror if anything goes wrong
        virtual auto open(const std::string &filePath) -> void override;

        /// @brief Get information about opened video file
        virtual auto getInfo() const -> VideoInfo override;

        /// @brief Read next XRGB8888 frame from video. Will return empty data if EOF
        virtual auto readFrame() -> std::vector<uint32_t> override;

        /// @brief Close reader opened with open()
        virtual auto close() -> void override;

    private:
        IO::Stream::FileHeader m_fileHeader;
        std::ifstream m_is;
    };

}
