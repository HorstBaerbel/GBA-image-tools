#include "vid2hio.h"

namespace IO
{

    auto Vid2h::writeVideoFrame(std::ostream &os, const Image::Data &frame) -> std::ostream &
    {
        static_assert(sizeof(FrameHeader) % 4 == 0);
        auto &imageData = frame.image.data;
        REQUIRE((imageData.pixels().rawSize() % 4) == 0, std::runtime_error, "Pixel data size is not a multiple of 4");
        REQUIRE((imageData.colorMap().rawSize() % 4) == 0, std::runtime_error, "Frame color map data size is not a multiple of 4");
        // convert pixel and color map data to raw format
        auto pixelData = imageData.pixels().convertDataToRaw();
        std::vector<uint8_t> colorMapData;
        if (imageData.pixels().isIndexed())
        {
            colorMapData = imageData.colorMap().convertDataToRaw();
        }
        // write pixel data frame header
        FrameHeader frameHeader;
        frameHeader.dataType = FrameType::Pixels;
        frameHeader.dataSize = pixelData.size();
        os.write(reinterpret_cast<const char *>(&frameHeader), sizeof(FrameHeader));
        REQUIRE(!os.fail(), std::runtime_error, "Failed to write pixel data frame header for frame #" << frame.index << " to stream");
        // write pixel data
        os.write(reinterpret_cast<const char *>(pixelData.data()), pixelData.size());
        REQUIRE(!os.fail(), std::runtime_error, "Failed to write pixel data for frame #" << frame.index << " to stream");
        // write color map data
        if (!colorMapData.empty())
        {
            // write pixel data frame header
            frameHeader.dataType = FrameType::Colormap;
            frameHeader.dataSize = colorMapData.size();
            os.write(reinterpret_cast<const char *>(&frameHeader), sizeof(FrameHeader));
            REQUIRE(!os.fail(), std::runtime_error, "Failed to write color map frame header for frame #" << frame.index << " to stream");
            // write color map data
            os.write(reinterpret_cast<const char *>(colorMapData.data()), colorMapData.size());
            REQUIRE(!os.fail(), std::runtime_error, "Failed to write color map data for frame #" << frame.index << " to stream");
        }
        return os;
    }

    auto Vid2h::writeAudioFrame(std::ostream &os, const std::vector<uint8_t> &frame, uint32_t index) -> std::ostream &
    {
        static_assert(sizeof(FrameHeader) % 4 == 0);
        REQUIRE((frame.size() % 4) == 0, std::runtime_error, "Audio data size is not a multiple of 4");
        // write audio data frame header
        FrameHeader frameHeader;
        frameHeader.dataType = FrameType::Audio;
        frameHeader.dataSize = frame.size();
        os.write(reinterpret_cast<const char *>(&frameHeader), sizeof(FrameHeader));
        REQUIRE(!os.fail(), std::runtime_error, "Failed to write audio data frame header for frame #" << index << " to stream");
        // write pixel data
        os.write(reinterpret_cast<const char *>(frame.data()), frame.size());
        REQUIRE(!os.fail(), std::runtime_error, "Failed to write pixel data for frame #" << index << " to stream");
        return os;
    }

    auto Vid2h::writeDummyFileHeader(std::ostream &os) -> std::ostream &
    {
        static_assert(sizeof(FileHeader) % 4 == 0);
        // generate dummy file header and store it
        FileHeader fileHeader;
        os.write(reinterpret_cast<const char *>(&fileHeader), sizeof(FileHeader));
        REQUIRE(!os.fail(), std::runtime_error, "Failed to write dummy file header to stream");
        return os;
    }

    auto Vid2h::writeFileHeader(std::ostream &os, const Image::ImageInfo &frameInfo, uint32_t nrOfFrames, double fps, uint32_t videoMemoryNeeded) -> std::ostream &
    {
        static_assert(sizeof(FileHeader) % 4 == 0);
        // check what color formats we're using
        const auto &pixelInfo = Color::formatInfo(frameInfo.pixelFormat);
        const auto &colorMapInfo = Color::formatInfo(frameInfo.colorMapFormat);
        // generate file header and store it
        FileHeader fileHeader;
        fileHeader.magic = IO::Vid2h::Magic;
        fileHeader.nrOfFrames = nrOfFrames;
        fileHeader.videoWidth = static_cast<uint16_t>(frameInfo.size.width());
        fileHeader.videoHeight = static_cast<uint16_t>(frameInfo.size.height());
        fileHeader.videoFps = static_cast<uint32_t>(std::round(fps * 65536.0));
        fileHeader.videoBitsPerPixel = static_cast<uint8_t>(pixelInfo.bitsPerPixel);
        fileHeader.videoBitsPerColor = pixelInfo.isIndexed ? static_cast<uint8_t>(colorMapInfo.bitsPerPixel) : 0;
        fileHeader.videoSwappedRedBlue = (pixelInfo.isIndexed ? colorMapInfo.hasSwappedRedBlue : pixelInfo.hasSwappedRedBlue) ? 1 : 0;
        fileHeader.videoColorMapEntries = pixelInfo.isIndexed ? frameInfo.nrOfColorMapEntries : 0;
        fileHeader.videoMemoryNeeded = videoMemoryNeeded;
        os.write(reinterpret_cast<const char *>(&fileHeader), sizeof(FileHeader));
        REQUIRE(!os.fail(), std::runtime_error, "Failed to write file header to stream");
        return os;
    }

    auto Vid2h::readFileHeader(std::istream &is) -> FileHeader
    {
        FileHeader fileHeader;
        is.read(reinterpret_cast<char *>(&fileHeader), sizeof(FileHeader));
        REQUIRE(!is.fail(), std::runtime_error, "Failed to read file header from stream");
        REQUIRE(fileHeader.magic == IO::Vid2h::Magic, std::runtime_error, "Bad file magic 0x" << std::hex << fileHeader.magic << " (expected 0x" << IO::Vid2h::Magic << ")");
        return fileHeader;
    }

    auto Vid2h::readFrame(std::istream &is, const FileHeader &fileHeader) -> std::pair<IO::FrameType, std::vector<uint8_t>>
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

    auto Vid2h::splitChunk(std::vector<uint8_t> &data) -> std::pair<ChunkHeader, std::vector<uint8_t>>
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
