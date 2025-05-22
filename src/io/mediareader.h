#pragma once

#include "mediatypes.h"
#include "color/xrgb8888.h"

#include <cstdint>
#include <string>
#include <vector>
#include <variant>

namespace Media
{

    /// @brief Media reader interface
    class Reader
    {
    public:
        /// @brief Information about opened media file
        struct MediaInfo
        {
            // ----- stream -----
            double durationS = 0;
            IO::FileType fileType = IO::FileType::Unknown;
            // ----- video -----
            uint64_t videoNrOfFrames = 0;
            std::string videoCodecName;
            uint32_t videoStreamIndex = 0;
            uint32_t videoWidth = 0;
            uint32_t videoHeight = 0;
            double videoFps = 0;
            Color::Format videoPixelFormat = Color::Format::Unknown;
            Color::Format videoColorMapFormat = Color::Format::Unknown;
            // ----- audio -----
            uint64_t audioNrOfFrames = 0;
            std::string audioCodecName;
            uint32_t audioStreamIndex = 0;
            uint32_t audioSampleRate = 0; // in Hz
            uint32_t audioChannels = 0;   // Only mono = 1 or stereo = 2 supported
            uint32_t audioSampleBits = 0; // Bits per sample. Always a signed data type, e.g. int16_t
        };

        /// @brief Raw frame data returned when reading a media stream
        struct FrameData
        {
            IO::FrameType frameType = IO::FrameType::Unknown;                      // Data type
            std::variant<std::vector<Color::XRGB8888>, std::vector<uint8_t>> data; // Raw, uncompressed pixel or audio data
        };

        /// @brief Destruktor. Calls close()
        virtual ~Reader();

        /// @brief Open reader on a file so you can later readFrame() from it
        /// @throw Throws a std::runtime_errror if anything goes wrong
        virtual auto open(const std::string &filePath) -> void = 0;

        /// @brief Get information about opened media file
        virtual auto getInfo() const -> MediaInfo = 0;

        /// @brief Read next video or audio frame. Will return FrameType::Unknown and empty data if EOF
        virtual auto readFrame() -> FrameData = 0;

        /// @brief Close reader opened with open()
        virtual auto close() -> void;
    };
}
