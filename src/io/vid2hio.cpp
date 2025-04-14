#include "vid2hio.h"

namespace IO
{

    const std::array<char, 4> Vid2h::VID2H_MAGIC = {'v', '2', 'h', '0'};

    auto Vid2h::writeFrame(std::ostream &os, const Image::Data &frame) -> std::ostream &
    {
        static_assert(sizeof(FrameHeader) % 4 == 0);
        auto &imageData = frame.image.data;
        REQUIRE((imageData.pixels().size() & 3) == 0, std::runtime_error, "Pixel data size is not a multiple of 4");
        REQUIRE((imageData.colorMap().size() & 3) == 0, std::runtime_error, "Frame color map data size is not a multiple of 4");
        // convert pixel and colo map data to raw format
        auto pixelData = imageData.pixels().convertDataToRaw();
        std::vector<uint8_t> colorMapData;
        if (imageData.pixels().isIndexed())
        {
            colorMapData = imageData.colorMap().convertDataToRaw();
        }
        // store frame header
        FrameHeader frameHeader;
        frameHeader.audioDataSize = 0;
        frameHeader.pixelDataSize = pixelData.size();
        frameHeader.colorMapDataSize = colorMapData.size();
        os.write(reinterpret_cast<const char *>(&frameHeader), sizeof(frameHeader));
        REQUIRE(!os.fail(), std::runtime_error, "Failed to write frame header for frame #" << frame.index << " to stream");
        // write audio data first
        // ...
        // write pixel data
        os.write(reinterpret_cast<const char *>(pixelData.data()), pixelData.size());
        REQUIRE(!os.fail(), std::runtime_error, "Failed to write pixel data for frame #" << frame.index << " to stream");
        // write color map data
        if (!colorMapData.empty())
        {
            os.write(reinterpret_cast<const char *>(colorMapData.data()), colorMapData.size());
            REQUIRE(!os.fail(), std::runtime_error, "Failed to write color map data for frame #" << frame.index << " to stream");
        }
        return os;
    }

    auto Vid2h::writeFrames(std::ostream &os, const std::vector<Image::Data> &frames) -> std::ostream &
    {
        for (const auto &f : frames)
        {
            writeFrame(os, f);
        }
        return os;
    }

    auto Vid2h::writeFileHeader(std::ostream &os, const std::vector<Image::Data> &frames, double fps, uint32_t videoMemoryNeeded) -> std::ostream &
    {
        static_assert(sizeof(FileHeader) % 4 == 0);
        const auto &frameData = frames.front().image.data;
        // check if we're using a color map
        const bool frameHasColorMap = frameData.pixels().isIndexed();
        // generate file header and store it
        FileHeader fileHeader;
        fileHeader.magic = VID2H_MAGIC;
        fileHeader.nrOfFrames = frames.size();
        fileHeader.width = static_cast<uint16_t>(frames.front().image.size.width());
        fileHeader.height = static_cast<uint16_t>(frames.front().image.size.height());
        fileHeader.fps = static_cast<uint32_t>(std::round(fps * 32768.0));
        fileHeader.bitsPerPixel = static_cast<uint8_t>(Color::formatInfo(frameData.pixels().format()).bitsPerPixel);
        fileHeader.bitsPerColor = frameHasColorMap ? static_cast<uint8_t>(Color::formatInfo(frameData.colorMap().format()).bitsPerPixel) : 0;
        fileHeader.swappedRedBlue = (frameHasColorMap ? Color::formatInfo(frameData.colorMap().format()).hasSwappedRedBlue : Color::formatInfo(frameData.pixels().format()).hasSwappedRedBlue) ? 1 : 0;
        fileHeader.colorMapEntries = frameHasColorMap ? frameData.colorMap().size() : 0;
        fileHeader.videoMemoryNeeded = videoMemoryNeeded;
        os.write(reinterpret_cast<const char *>(&fileHeader), sizeof(fileHeader));
        REQUIRE(!os.fail(), std::runtime_error, "Failed to write file header to stream");
        return os;
    }

    auto Vid2h::readFileHeader(std::istream &is) -> FileHeader
    {
        REQUIRE((sizeof(FileHeader) & 3) == 0, std::runtime_error, "FileHeader size is not a multiple of 4");
        FileHeader fileHeader;
        is.read(reinterpret_cast<char *>(&fileHeader), sizeof(FileHeader));
        REQUIRE(!is.fail(), std::runtime_error, "Failed to read file header from stream");
        REQUIRE(fileHeader.magic == VID2H_MAGIC, std::runtime_error, "Bad file magic " << fileHeader.magic.at(0) << fileHeader.magic.at(1) << fileHeader.magic.at(2) << fileHeader.magic.at(3));
        return fileHeader;
    }

    auto Vid2h::readFrame(std::istream &is, const FileHeader &fileHeader) -> FrameData
    {
        // read frame header
        FrameHeader frameHeader;
        is.read(reinterpret_cast<char *>(&frameHeader), sizeof(frameHeader));
        REQUIRE(!is.fail(), std::runtime_error, "Failed to read frame header from stream");
        // allocate memory
        FrameData frameData;
        frameData.audioData.resize(frameHeader.audioDataSize);
        frameData.pixelData.resize(frameHeader.pixelDataSize);
        frameData.colorMapData.resize(frameHeader.colorMapDataSize);
        // read audio data
        if (frameHeader.audioDataSize != 0)
        {
            is.read(reinterpret_cast<char *>(frameData.audioData.data()), frameData.audioData.size());
            REQUIRE(!is.fail(), std::runtime_error, "Failed to read audio data from stream");
        }
        // read pixel data
        if (frameHeader.pixelDataSize != 0)
        {
            is.read(reinterpret_cast<char *>(frameData.pixelData.data()), frameData.pixelData.size());
            REQUIRE(!is.fail(), std::runtime_error, "Failed to read pixel data from stream");
        }
        // read color map data
        if (frameHeader.colorMapDataSize != 0)
        {
            is.read(reinterpret_cast<char *>(frameData.colorMapData.data()), frameData.colorMapData.size());
            REQUIRE(!is.fail(), std::runtime_error, "Failed to read color map data from stream");
        }
        return frameData;
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
