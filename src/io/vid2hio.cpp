#include "vid2hio.h"

#include "audio/audiohelpers.h"

namespace IO::Vid2h
{

    auto writeFrame(std::ostream &os, const Image::Frame &frame) -> std::ostream &
    {
        static_assert(sizeof(FrameHeader) % 4 == 0);
        auto &imageData = frame.data;
        REQUIRE((imageData.pixels().rawSize() % 4) == 0, std::runtime_error, "Pixel data size is not a multiple of 4");
        REQUIRE((imageData.colorMap().rawSize() % 4) == 0, std::runtime_error, "Frame color map data size is not a multiple of 4");
        // convert pixel and color map data to raw format
        auto pixelData = imageData.pixels().convertDataToRaw();
        std::vector<uint8_t> colorMapData;
        if (imageData.pixels().isIndexed())
        {
            colorMapData = imageData.colorMap().convertDataToRaw();
        }
        // write color map data first
        FrameHeader frameHeader;
        if (!colorMapData.empty())
        {
            frameHeader.dataType = FrameType::Colormap;
            frameHeader.dataSize = colorMapData.size();
            os.write(reinterpret_cast<const char *>(&frameHeader), sizeof(FrameHeader));
            REQUIRE(!os.fail(), std::runtime_error, "Failed to write color map frame header for frame #" << frame.index << " to stream");
            os.write(reinterpret_cast<const char *>(colorMapData.data()), colorMapData.size());
            REQUIRE(!os.fail(), std::runtime_error, "Failed to write color map data for frame #" << frame.index << " to stream");
        }
        // write pixel data after that
        frameHeader.dataType = FrameType::Pixels;
        frameHeader.dataSize = pixelData.size();
        os.write(reinterpret_cast<const char *>(&frameHeader), sizeof(FrameHeader));
        REQUIRE(!os.fail(), std::runtime_error, "Failed to write pixel data frame header for frame #" << frame.index << " to stream");
        os.write(reinterpret_cast<const char *>(pixelData.data()), pixelData.size());
        REQUIRE(!os.fail(), std::runtime_error, "Failed to write pixel data for frame #" << frame.index << " to stream");
        return os;
    }

    auto writeFrame(std::ostream &os, const Audio::Frame &frame) -> std::ostream &
    {
        static_assert(sizeof(FrameHeader) % 4 == 0);
        auto sampleData = AudioHelpers::toRawData(frame.data, frame.info.channelFormat);
        REQUIRE((sampleData.size() % 4) == 0, std::runtime_error, "Audio data size is not a multiple of 4");
        // write audio data frame header
        FrameHeader frameHeader;
        frameHeader.dataType = FrameType::Audio;
        frameHeader.dataSize = sampleData.size();
        os.write(reinterpret_cast<const char *>(&frameHeader), sizeof(FrameHeader));
        REQUIRE(!os.fail(), std::runtime_error, "Failed to write audio data frame header for frame #" << frame.index << " to stream");
        // write audio sample data
        os.write(reinterpret_cast<const char *>(sampleData.data()), sampleData.size());
        REQUIRE(!os.fail(), std::runtime_error, "Failed to write audio data for frame #" << frame.index << " to stream");
        return os;
    }

    auto writeDummyFileHeader(std::ostream &os) -> std::ostream &
    {
        static_assert(sizeof(FileHeader) % 4 == 0);
        // generate dummy file header and store it
        FileHeader fileHeader;
        os.write(reinterpret_cast<const char *>(&fileHeader), sizeof(FileHeader));
        REQUIRE(!os.fail(), std::runtime_error, "Failed to write dummy file header to stream");
        return os;
    }

    auto createVideoHeader(const Image::FrameInfo &imageInfo, uint32_t videoNrOfFrames, double videoFrameRateHz, uint32_t videoMemoryNeeded, uint32_t videoNrOfColorMapFrames, const std::vector<Image::ProcessingType> &decodingSteps) -> VideoHeader
    {
        REQUIRE(videoNrOfFrames < 2 ^ 16, std::runtime_error, "Number of video frames must be < 2^16");
        REQUIRE(videoMemoryNeeded < 2 ^ 24, std::runtime_error, "Max. video memory needed must be < 2^24");
        REQUIRE(videoNrOfColorMapFrames < 2 ^ 16, std::runtime_error, "Number of color map frames must be < 2^16");
        VideoHeader outHeader;
        // add video information
        const auto &pixelInfo = Color::formatInfo(imageInfo.pixelFormat);
        const auto &colorMapInfo = Color::formatInfo(imageInfo.colorMapFormat);
        outHeader.nrOfFrames = videoNrOfFrames;
        outHeader.frameRateHz = static_cast<uint32_t>(std::round(videoFrameRateHz * 65536.0));
        outHeader.width = static_cast<uint16_t>(imageInfo.size.width());
        outHeader.height = static_cast<uint16_t>(imageInfo.size.height());
        outHeader.bitsPerPixel = static_cast<uint8_t>(pixelInfo.bitsPerPixel);
        outHeader.bitsPerColor = pixelInfo.isIndexed ? static_cast<uint8_t>(colorMapInfo.bitsPerPixel) : 0;
        outHeader.colorMapEntries = pixelInfo.isIndexed ? imageInfo.nrOfColorMapEntries : 0;
        outHeader.nrOfColorMapFrames = videoNrOfColorMapFrames;
        outHeader.swappedRedBlue = (pixelInfo.isIndexed ? colorMapInfo.hasSwappedRedBlue : pixelInfo.hasSwappedRedBlue) ? 1 : 0;
        outHeader.memoryNeeded = videoMemoryNeeded;
        REQUIRE(decodingSteps.size() <= 4, std::runtime_error, "Number of decoding steps must be <= 4");
        std::memcpy(outHeader.processing, decodingSteps.data(), decodingSteps.size());
        return outHeader;
    }

    auto createAudioHeader(const Audio::FrameInfo &audioInfo, uint32_t audioNrOfFrames, uint32_t audioNrOfSamples, int32_t audioOffsetSamples, uint32_t audioMemoryNeeded, const std::vector<Audio::ProcessingType> &decodingSteps) -> AudioHeader
    {
        REQUIRE(audioNrOfFrames < 2 ^ 16, std::runtime_error, "Number of audio frames must be < 2^16");
        REQUIRE(audioMemoryNeeded < 2 ^ 16, std::runtime_error, "Max. audio memory needed must be < 2^16");
        AudioHeader outHeader;
        // add audio information
        const auto &channelInfo = Audio::formatInfo(audioInfo.channelFormat);
        const auto &sampleInfo = Audio::formatInfo(audioInfo.sampleFormat);
        outHeader.nrOfFrames = audioNrOfFrames;
        outHeader.nrOfSamples = audioNrOfSamples;
        outHeader.sampleRateHz = audioInfo.sampleRateHz;
        outHeader.channels = channelInfo.nrOfChannels;
        outHeader.sampleBits = sampleInfo.bitsPerSample;
        REQUIRE(audioOffsetSamples >= std::numeric_limits<int16_t>::min() && audioOffsetSamples <= std::numeric_limits<int16_t>::max(), std::runtime_error, "Audio offset needed must be in [" << std::numeric_limits<int16_t>::min() << "," << std::numeric_limits<int16_t>::max() << "]");
        outHeader.offsetSamples = audioOffsetSamples;
        REQUIRE(audioMemoryNeeded <= std::numeric_limits<uint16_t>::max(), std::runtime_error, "Audio memory needed must be <= " << std::numeric_limits<uint16_t>::max());
        outHeader.memoryNeeded = audioMemoryNeeded;
        REQUIRE(decodingSteps.size() <= 4, std::runtime_error, "Number of decoding steps must be <= 4");
        std::memcpy(outHeader.processing, decodingSteps.data(), decodingSteps.size());
        return outHeader;
    }

    auto writeMediaFileHeader(std::ostream &os, const VideoHeader &videoHeader, const AudioHeader &audioHeader) -> std::ostream &
    {
        static_assert(sizeof(FileHeader) % 4 == 0);
        // generate file header
        FileHeader fileHeader;
        fileHeader.magic = IO::Vid2h::Magic;
        fileHeader.contentType = IO::FileType::AudioVideo;
        fileHeader.audio = audioHeader;
        fileHeader.video = videoHeader;
        // store to file
        os.write(reinterpret_cast<const char *>(&fileHeader), sizeof(FileHeader));
        REQUIRE(!os.fail(), std::runtime_error, "Failed to write media file header to stream");
        return os;
    }

    auto writeVideoFileHeader(std::ostream &os, const VideoHeader &videoHeader) -> std::ostream &
    {
        static_assert(sizeof(FileHeader) % 4 == 0);
        // generate file header
        FileHeader fileHeader;
        fileHeader.magic = IO::Vid2h::Magic;
        fileHeader.contentType = IO::FileType::Video;
        fileHeader.video = videoHeader;
        // store to file
        os.write(reinterpret_cast<const char *>(&fileHeader), sizeof(FileHeader));
        REQUIRE(!os.fail(), std::runtime_error, "Failed to write video file header to stream");
        return os;
    }

    auto writeAudioFileHeader(std::ostream &os, const AudioHeader &audioHeader) -> std::ostream &
    {
        static_assert(sizeof(FileHeader) % 4 == 0);
        // generate file header
        FileHeader fileHeader;
        fileHeader.magic = IO::Vid2h::Magic;
        fileHeader.contentType = IO::FileType::Audio;
        fileHeader.audio = audioHeader;
        // store to file
        os.write(reinterpret_cast<const char *>(&fileHeader), sizeof(FileHeader));
        REQUIRE(!os.fail(), std::runtime_error, "Failed to write media file header to stream");
        return os;
    }

    auto readFileHeader(std::istream &is) -> FileHeader
    {
        FileHeader fileHeader;
        is.read(reinterpret_cast<char *>(&fileHeader), sizeof(FileHeader));
        REQUIRE(!is.fail(), std::runtime_error, "Failed to read file header from stream");
        REQUIRE(fileHeader.magic == IO::Vid2h::Magic, std::runtime_error, "Bad file magic 0x" << std::hex << fileHeader.magic << " (expected 0x" << IO::Vid2h::Magic << ")");
        return fileHeader;
    }

    auto readFrame(std::istream &is, const FileHeader &fileHeader) -> std::pair<IO::FrameType, std::vector<uint8_t>>
    {
        // read frame header
        FrameHeader frameHeader;
        is.read(reinterpret_cast<char *>(&frameHeader), sizeof(FrameHeader));
        REQUIRE(!is.fail(), std::runtime_error, "Failed to read frame header from stream");
        // allocate memory
        std::vector<uint8_t> frameData;
        frameData.resize(frameHeader.dataSize);
        // read data
        is.read(reinterpret_cast<char *>(frameData.data()), frameData.size());
        if (is.fail())
        {
            switch (frameHeader.dataType)
            {
            case FrameType::Pixels:
                THROW(std::runtime_error, "Failed to read pixel data from stream");
            case FrameType::Colormap:
                THROW(std::runtime_error, "Failed to read color map data from stream");
            case FrameType::Audio:
                THROW(std::runtime_error, "Failed to read audio data from stream");
            default:
                THROW(std::runtime_error, "Got bad data type " << static_cast<uint32_t>(frameHeader.dataType) << " from stream");
            }
        }
        return {IO::FrameType(frameHeader.dataType), frameData};
    }
}
