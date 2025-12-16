#pragma once

#include "audio/audiostructs.h"
#include "color/xrgb8888.h"
#include "if/mediatypes.h"
#include "image/imagestructs.h"
#include "subtitles/subtitlesstructs.h"

#include <cstdint>
#include <memory>
#include <string>
#include <variant>
#include <vector>

namespace Media
{

    /// @brief Media reader interface
    class Reader
    {
    public:
        using SPtr = std::shared_ptr<Reader>;

        /// @brief Information about opened media file
        struct MediaInfo
        {
            // ----- stream -----
            IO::FileType fileType = IO::FileType::Unknown;
            // ----- video -----
            uint32_t videoNrOfFrames = 0; // Number of all video frames (must not be the same as audio frames)
            double videoFrameRateHz = 0;  // Vide frame rate in Hz
            double videoDurationS = 0;    // Video runtime in s
            std::string videoCodecName;
            uint32_t videoStreamIndex = 0;
            uint32_t videoWidth = 0;
            uint32_t videoHeight = 0;
            Color::Format videoPixelFormat = Color::Format::Unknown;
            Color::Format videoColorMapFormat = Color::Format::Unknown;
            // ----- audio -----
            uint32_t audioNrOfFrames = 0;  // Number of all audio frames (must not be the same as video frames)
            uint32_t audioNrOfSamples = 0; // Number of samples per channel
            double audioDurationS = 0;     // Audio runtime in s
            std::string audioCodecName;
            uint32_t audioStreamIndex = 0;
            uint32_t audioSampleRateHz = 0;                                          // Sample rate in Hz
            Audio::ChannelFormat audioChannelFormat = Audio::ChannelFormat::Unknown; // Only mono = 1 or stereo = 2 supported
            Audio::SampleFormat audioSampleFormat = Audio::SampleFormat::Unknown;    // Description of bits and signed / unsigned in sample format
            double audioOffsetS = 0;                                                 // Offset of audio relative to video in s
            // ----- subtitles -----
            uint32_t subtitlesNrOfFrames = 0; // Number of all subtitles frames (must not be the same as video/audio frames)
            // ----- meta data -----
            uint32_t metaDataSize = 0;
        };

        /// @brief Raw frame data returned when reading a media stream
        struct FrameData
        {
            IO::FrameType frameType = IO::FrameType::Unknown;                      // Data type
            double presentTimeInS = 0;                                             // Presentation timestamp in seconds
            std::variant<Image::RawData, Audio::RawData, Subtitles::RawData> data; // Raw pixel or (planar) audio or subtitles data
        };

        /// @brief Default constructor.
        Reader() = default;

        /// @brief Destructor. Calls close()
        virtual ~Reader();

        /// @brief Open reader on a file so you can later readFrame() from it
        /// @throw Throws a std::runtime_errror if anything goes wrong
        virtual auto open(const std::string &filePath) -> void = 0;

        /// @brief Get information about opened media file
        virtual auto getInfo() const -> MediaInfo = 0;

        /// @brief Get unstructured meta data from opened video file
        virtual auto getMetaData() const -> std::vector<uint8_t>;

        /// @brief Read next video or audio frame. Will return FrameType::Unknown and empty data if EOF
        virtual auto readFrame() -> FrameData = 0;

        /// @brief Close reader opened with open()
        virtual auto close() -> void;

    private:
        Reader(const Reader &) = delete;            // non construction-copyable
        Reader &operator=(const Reader &) = delete; // non copyable
    };
}
