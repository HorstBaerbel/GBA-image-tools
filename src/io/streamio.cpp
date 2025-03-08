#include "streamio.h"

namespace IO
{

    const std::array<char, 4> Stream::VID2H_MAGIC = {'v', '2', 'h', '_'};

    auto Stream::writeFrame(std::ostream &os, const Image::Data &frame) -> std::ostream &
    {
        REQUIRE((frame.image.data.pixels().size() & 3) == 0, std::runtime_error, "Frame data size is not a multiple of 4");
        REQUIRE((frame.image.data.colorMap().size() & 3) == 0, std::runtime_error, "Frame color map data size is not a multiple of 4");
        // store compressed frame data size. this might include a prepended processing type / uncompressed size
        auto frameData = frame.image.data.pixels().convertDataToRaw();
        const uint32_t frameSize = frameData.size();
        os.write(reinterpret_cast<const char *>(&frameSize), sizeof(frameSize));
        REQUIRE(!os.fail(), std::runtime_error, "Failed to write frame size for frame #" << frame.index << " to stream");
        // write frame data
        os.write(reinterpret_cast<const char *>(frameData.data()), frameData.size());
        REQUIRE(!os.fail(), std::runtime_error, "Failed to write pixel data for frame #" << frame.index << " to stream");
        // check if we're using a color map and write that
        if (frame.image.data.pixels().isIndexed())
        {
            auto colorMapData = frame.image.data.colorMap().convertDataToRaw();
            os.write(reinterpret_cast<const char *>(colorMapData.data()), colorMapData.size());
            REQUIRE(!os.fail(), std::runtime_error, "Failed to write color map data for frame #" << frame.index << " to stream");
        }
        return os;
    }

    auto Stream::writeFrames(std::ostream &os, const std::vector<Image::Data> &frames) -> std::ostream &
    {
        for (const auto &f : frames)
        {
            writeFrame(os, f);
        }
        return os;
    }

    auto Stream::writeFileHeader(std::ostream &os, const std::vector<Image::Data> &frames, double fps, uint32_t maxMemoryNeeded) -> std::ostream &
    {
        REQUIRE((sizeof(FileHeader) & 3) == 0, std::runtime_error, "FileHeader size is not a multiple of 4");
        const auto &frameData = frames.front().image.data;
        // check if we're using a color map
        const bool frameHasColorMap = frameData.pixels().isIndexed();
        // generate file header and store it
        FileHeader fileHeader;
        fileHeader.magic = VID2H_MAGIC;
        fileHeader.nrOfFrames = frames.size();
        fileHeader.width = static_cast<uint16_t>(frames.front().image.size.width());
        fileHeader.height = static_cast<uint16_t>(frames.front().image.size.height());
        fileHeader.fps = static_cast<uint32_t>(std::round(fps * 65536.0));
        fileHeader.bitsPerPixel = static_cast<uint8_t>(Color::formatInfo(frameData.pixels().format()).bitsPerPixel);
        fileHeader.bitsPerColor = frameHasColorMap ? static_cast<uint8_t>(Color::formatInfo(frameData.colorMap().format()).bitsPerPixel) : 0;
        fileHeader.swappedRedBlue = (frameHasColorMap ? Color::formatInfo(frameData.colorMap().format()).hasSwappedRedBlue : Color::formatInfo(frameData.pixels().format()).hasSwappedRedBlue) ? 1 : 0;
        fileHeader.colorMapEntries = frameHasColorMap ? frameData.colorMap().size() : 0;
        fileHeader.maxMemoryNeeded = maxMemoryNeeded;
        os.write(reinterpret_cast<const char *>(&fileHeader), sizeof(fileHeader));
        REQUIRE(!os.fail(), std::runtime_error, "Failed to write file header to stream");
        return os;
    }

    auto Stream::readFileHeader(std::istream &is) -> FileHeader
    {
        REQUIRE((sizeof(FileHeader) & 3) == 0, std::runtime_error, "FileHeader size is not a multiple of 4");
        FileHeader fileHeader;
        is.read(reinterpret_cast<char *>(&fileHeader), sizeof(FileHeader));
        REQUIRE(!is.fail(), std::runtime_error, "Failed to read file header from stream");
        return fileHeader;
    }

    auto Stream::readFrame(std::istream &is, const FileHeader &fileHeader) -> std::pair<std::vector<uint8_t>, std::vector<uint8_t>>
    {
        uint32_t frameSize = 0;
        is.read(reinterpret_cast<char *>(&frameSize), sizeof(frameSize));
        REQUIRE(!is.fail(), std::runtime_error, "Failed to read frame size from stream");
        const bool frameHasColorMap = fileHeader.colorMapEntries != 0;
        const uint32_t frameDataSize = frameHasColorMap ? frameSize - fileHeader.colorMapEntries * ((fileHeader.bitsPerColor + 7) / 8) : frameSize;
        const uint32_t colorMapDataSize = frameSize - frameDataSize;
        std::vector<uint8_t> frameData(frameDataSize);
        std::vector<uint8_t> colorMapData(colorMapDataSize);
        is.read(reinterpret_cast<char *>(frameData.data()), frameData.size());
        REQUIRE(!is.fail(), std::runtime_error, "Failed to read pixel data from stream");
        if (frameHasColorMap)
        {
            is.read(reinterpret_cast<char *>(colorMapData.data()), colorMapData.size());
            REQUIRE(!is.fail(), std::runtime_error, "Failed to read color map data from stream");
        }
        return {frameData, colorMapData};
    }
}
