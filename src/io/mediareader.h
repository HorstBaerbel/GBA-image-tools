#pragma once

#include "audio/audioformat.h"
#include "color/xrgb8888.h"
#include "mediatypes.h"

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
            IO::FileType fileType = IO::FileType::Unknown;
            // ----- video -----
            uint32_t videoNrOfFrames = 0; // Numbver of all video frames in file (must not be the same as audio frames)
            double videoFrameRateHz = 0;  // Vide frame rate in Hz
            double videoDurationS = 0;    // Video runtime in s
            std::string videoCodecName;
            uint32_t videoStreamIndex = 0;
            uint32_t videoWidth = 0;
            uint32_t videoHeight = 0;
            Color::Format videoPixelFormat = Color::Format::Unknown;
            Color::Format videoColorMapFormat = Color::Format::Unknown;
            // ----- audio -----
            uint32_t audioNrOfFrames = 0;  // Numbver of all audio frames in file (must not be the same as video frames)
            uint32_t audioNrOfSamples = 0; // Number of all samples in file
            double audioDurationS = 0;     // Audio runtime in s
            std::string audioCodecName;
            uint32_t audioStreamIndex = 0;
            uint32_t audioSampleRateHz = 0; // Sample rate in Hz
            Audio::ChannelFormat audioChannelFormat = Audio::ChannelFormat::Unknown; // Only mono = 1 or stereo = 2 supported
            Audio::SampleFormat audioSampleFormat = Audio::SampleFormat::Unknown; // Description of bits and signed / unsigned in sample format
            double audioOffsetS = 0;                                              // Offset of audio relative to video in s
        };

        /// @brief Raw frame data returned when reading a media stream
        struct FrameData
        {
            IO::FrameType frameType = IO::FrameType::Unknown;                      // Data type
            std::variant<std::vector<Color::XRGB8888>, std::vector<int16_t>> data; // Raw, uncompressed pixel or audio data
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
