#include "streamio.h"

namespace IO
{

    auto Stream::writeFrame(std::ostream &os, const Image::Data &frame) -> std::ostream &
    {
        REQUIRE((frame.imageData.pixels().size() & 3) == 0, std::runtime_error, "Frame data size is not a multiple of 4");
        REQUIRE((frame.imageData.colorMap().size() & 3) == 0, std::runtime_error, "Frame color map data size is not a multiple of 4");
        // store compressed frame data size. this might include a prepended processing type / uncompressed size
        auto frameData = frame.imageData.pixels().convertDataToRaw();
        const uint32_t frameSize = frameData.size();
        os.write(reinterpret_cast<const char *>(&frameSize), sizeof(frameSize));
        // write frame data
        os.write(reinterpret_cast<const char *>(frameData.data()), frameData.size());
        // check if we're using a color map and write that
        if (frame.imageData.pixels().isIndexed())
        {
            auto colorMapData = frame.imageData.colorMap().convertDataToRaw();
            os.write(reinterpret_cast<const char *>(colorMapData.data()), colorMapData.size());
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

    auto Stream::writeFileHeader(std::ostream &os, const std::vector<Image::Data> &frames, uint8_t fps, uint32_t maxMemoryNeeded) -> std::ostream &
    {
        REQUIRE((sizeof(FileHeader) & 3) == 0, std::runtime_error, "FileHeader size is not a multiple of 4");
        const auto &frameData = frames.front().imageData;
        // check if we're using a color map
        const bool frameHasColorMap = frameData.pixels().isIndexed();
        // generate file header and store it
        FileHeader fileHeader;
        fileHeader.nrOfFrames = frames.size();
        fileHeader.width = static_cast<uint16_t>(frames.front().size.width());
        fileHeader.height = static_cast<uint16_t>(frames.front().size.height());
        fileHeader.fps = fps;
        fileHeader.bitsPerPixel = Color::bitsPerPixelForFormat(frameData.pixels().format());
        fileHeader.bitsPerColor = frameHasColorMap ? Color::bitsPerPixelForFormat(frameData.colorMap().format()) : 0;
        fileHeader.colorMapEntries = frameHasColorMap ? frameData.colorMap().size() : 0;
        fileHeader.maxMemoryNeeded = maxMemoryNeeded;
        os.write(reinterpret_cast<const char *>(&fileHeader), sizeof(fileHeader));
        return os;
    }

}
