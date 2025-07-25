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

    auto addInfoToFileHeader(const FileHeader &inHeader, const Image::FrameInfo &imageInfo, uint32_t videoNrOfFrames, double videoFrameRateHz, uint32_t videoMemoryNeeded) -> FileHeader
    {
        FileHeader outHeader = inHeader;
        // add video information
        const auto &pixelInfo = Color::formatInfo(imageInfo.pixelFormat);
        const auto &colorMapInfo = Color::formatInfo(imageInfo.colorMapFormat);
        outHeader.videoNrOfFrames = videoNrOfFrames;
        outHeader.videoFrameRateHz = static_cast<uint32_t>(std::round(videoFrameRateHz * 65536.0));
        outHeader.videoWidth = static_cast<uint16_t>(imageInfo.size.width());
        outHeader.videoHeight = static_cast<uint16_t>(imageInfo.size.height());
        outHeader.videoBitsPerPixel = static_cast<uint8_t>(pixelInfo.bitsPerPixel);
        outHeader.videoBitsPerColor = pixelInfo.isIndexed ? static_cast<uint8_t>(colorMapInfo.bitsPerPixel) : 0;
        outHeader.videoSwappedRedBlue = (pixelInfo.isIndexed ? colorMapInfo.hasSwappedRedBlue : pixelInfo.hasSwappedRedBlue) ? 1 : 0;
        outHeader.videoColorMapEntries = pixelInfo.isIndexed ? imageInfo.nrOfColorMapEntries : 0;
        outHeader.videoMemoryNeeded = videoMemoryNeeded;
        return outHeader;
    }

    auto addInfoToFileHeader(const FileHeader &inHeader, const Audio::FrameInfo &audioInfo, uint32_t audioNrOfFrames, uint32_t audioNrOfSamples, int32_t audioOffsetSamples, uint32_t audioMemoryNeeded) -> FileHeader
    {
        FileHeader outHeader = inHeader;
        // add audio information
        const auto &channelInfo = Audio::formatInfo(audioInfo.channelFormat);
        const auto &sampleInfo = Audio::formatInfo(audioInfo.sampleFormat);
        outHeader.audioNrOfFrames = audioNrOfFrames;
        outHeader.audioNrOfSamples = audioNrOfSamples;
        outHeader.audioSampleRateHz = audioInfo.sampleRateHz;
        outHeader.audioChannels = channelInfo.nrOfChannels;
        outHeader.audioSampleBits = sampleInfo.bitsPerSample;
        REQUIRE(audioOffsetSamples >= std::numeric_limits<int16_t>::min() && audioOffsetSamples <= std::numeric_limits<int16_t>::max(), std::runtime_error, "Audio offset needed must be in [" << std::numeric_limits<int16_t>::min() << "," << std::numeric_limits<int16_t>::max() << "]");
        outHeader.audioOffsetSamples = audioOffsetSamples;
        REQUIRE(audioMemoryNeeded <= std::numeric_limits<uint16_t>::max(), std::runtime_error, "Audio memory needed must be <= " << std::numeric_limits<uint16_t>::max());
        outHeader.audioMemoryNeeded = audioMemoryNeeded;
        return outHeader;
    }

    auto writeMediaFileHeader(std::ostream &os, const Image::FrameInfo &imageInfo, uint32_t videoNrOfFrames, double videoFrameRateHz, uint32_t videoMemoryNeeded, const Audio::FrameInfo &audioInfo, uint32_t audioNrOfFrames, uint32_t audioNrOfSamples, int32_t audioOffsetSamples, uint32_t audioMemoryNeeded) -> std::ostream &
    {
        static_assert(sizeof(FileHeader) % 4 == 0);
        // generate file header
        FileHeader fileHeader;
        fileHeader.magic = IO::Vid2h::Magic;
        fileHeader.contentType = IO::FileType::AudioVideo;
        // add video information
        fileHeader = addInfoToFileHeader(fileHeader, imageInfo, videoNrOfFrames, videoFrameRateHz, videoMemoryNeeded);
        // add audio information
        fileHeader = addInfoToFileHeader(fileHeader, audioInfo, audioNrOfFrames, audioNrOfSamples, audioOffsetSamples, audioMemoryNeeded);
        // store to file
        os.write(reinterpret_cast<const char *>(&fileHeader), sizeof(FileHeader));
        REQUIRE(!os.fail(), std::runtime_error, "Failed to write media file header to stream");
        return os;
    }

    auto writeVideoFileHeader(std::ostream &os, const Image::FrameInfo &imageInfo, uint32_t videoNrOfFrames, double videoFrameRateHz, uint32_t videoMemoryNeeded) -> std::ostream &
    {
        static_assert(sizeof(FileHeader) % 4 == 0);
        // generate file header
        FileHeader fileHeader;
        fileHeader.magic = IO::Vid2h::Magic;
        fileHeader.contentType = IO::FileType::Video;
        // add video information
        fileHeader = addInfoToFileHeader(fileHeader, imageInfo, videoNrOfFrames, videoFrameRateHz, videoMemoryNeeded);
        // store to file
        os.write(reinterpret_cast<const char *>(&fileHeader), sizeof(FileHeader));
        REQUIRE(!os.fail(), std::runtime_error, "Failed to write video file header to stream");
        return os;
    }

    auto writeAudioFileHeader(std::ostream &os, const Audio::FrameInfo &audioInfo, uint32_t audioNrOfFrames, uint32_t audioNrOfSamples, int32_t audioOffsetSamples, uint32_t audioMemoryNeeded) -> std::ostream &
    {
        static_assert(sizeof(FileHeader) % 4 == 0);
        // generate file header
        FileHeader fileHeader;
        fileHeader.magic = IO::Vid2h::Magic;
        fileHeader.contentType = IO::FileType::Audio;
        // add audio information
        fileHeader = addInfoToFileHeader(fileHeader, audioInfo, audioNrOfFrames, audioNrOfSamples, audioOffsetSamples, audioMemoryNeeded);
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

    auto splitChunk(std::vector<uint8_t> &data) -> std::pair<ChunkHeader, std::vector<uint8_t>>
    {
        static_assert(sizeof(ChunkHeader) % 4 == 0);
        REQUIRE(data.size() > sizeof(ChunkHeader), std::runtime_error, "Bad data size");
        ChunkHeader outHeader;
        auto dataHeader = reinterpret_cast<const ChunkHeader *>(data.data());
        outHeader.processingType = dataHeader->processingType;
        outHeader.uncompressedSize = dataHeader->uncompressedSize;
        std::vector<uint8_t> outData(data.size() - sizeof(ChunkHeader));
        std::copy(std::next(data.cbegin(), sizeof(ChunkHeader)), data.cend(), outData.begin());
        return {outHeader, outData};
    }
}
