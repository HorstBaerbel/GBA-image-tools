#include "streamio.h"

namespace IO
{

    auto Stream::writeFrame(std::ostream &os, const Image::Data &frame) -> std::ostream &
    {
        REQUIRE((frame.data.size() & 3) == 0, std::runtime_error, "Frame data size is not a multiple of 4");
        REQUIRE((frame.colorMapData.size() & 3) == 0, std::runtime_error, "Frame color map data size is not a multiple of 4");
        // store compressed frame data size. this might include a prepended processing type / uncompressed size
        const uint32_t frameSize = frame.data.size();
        os.write(reinterpret_cast<const char *>(&frameSize), sizeof(frameSize));
        // write frame data
        os.write(reinterpret_cast<const char *>(frame.data.data()), frame.data.size());
        // check if we're using a color map and write that
        if (hasColorMap(frame))
        {
            os.write(reinterpret_cast<const char *>(frame.colorMapData.data()), frame.colorMapData.size());
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
        // check if we're using a color map
        const bool frameHasColorMap = Image::hasColorMap(frames.front());
        // generate file header and store it
        FileHeader fileHeader;
        fileHeader.nrOfFrames = frames.size();
        fileHeader.width = static_cast<uint16_t>(frames.front().size.width());
        fileHeader.height = static_cast<uint16_t>(frames.front().size.height());
        fileHeader.fps = fps;
        fileHeader.bitsPerPixel = bitsPerPixelForFormat(frames.front().colorFormat);
        fileHeader.bitsPerColor = frameHasColorMap ? bitsPerPixelForFormat(frames.front().colorMapFormat) : 0;
        fileHeader.colorMapEntries = frameHasColorMap ? frames.front().colorMap.size() : 0;
        fileHeader.maxMemoryNeeded = maxMemoryNeeded;
        os.write(reinterpret_cast<const char *>(&fileHeader), sizeof(fileHeader));
        return os;
    }

}
