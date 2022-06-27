#include "imageio.h"

namespace Image
{

    bool hasColorMap(const Data &frame)
    {
        switch (frame.colorMapFormat)
        {
        case ColorFormat::Unknown:
            return false;
            break;
        case ColorFormat::RGB555:
        case ColorFormat::RGB565:
        case ColorFormat::RGB888:
            return frame.colorMap.size() > 0;
            break;
        default:
            THROW(std::runtime_error, "Unsupported color map format");
            break;
        }
    }

    uint32_t bytesPerColorMapEntry(const Data &frame)
    {
        switch (frame.colorMapFormat)
        {
        case ColorFormat::Unknown:
            return 0;
            break;
        case ColorFormat::RGB555:
        case ColorFormat::RGB565:
            return 2;
        case ColorFormat::RGB888:
            return 3;
            break;
        default:
            THROW(std::runtime_error, "Unsupported color map format");
            break;
        }
    }

    auto IO::writeFrame(std::ostream &os, const Data &frame) -> std::ostream &
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

    auto IO::writeFrames(std::ostream &os, const std::vector<Data> &frames) -> std::ostream &
    {
        for (const auto &f : frames)
        {
            writeFrame(os, f);
        }
        return os;
    }

    auto IO::writeFileHeader(std::ostream &os, const std::vector<Data> &frames, uint8_t fps, uint32_t maxMemoryNeeded) -> std::ostream &
    {
        REQUIRE((sizeof(FileHeader) & 3) == 0, std::runtime_error, "FileHeader size is not a multiple of 4");
        // check if we're using a color map
        const bool frameHasColorMap = hasColorMap(frames.front());
        // generate file header and store it
        FileHeader fileHeader;
        fileHeader.nrOfFrames = frames.size();
        fileHeader.width = frames.front().size.width();
        fileHeader.height = frames.front().size.height();
        fileHeader.fps = fps;
        fileHeader.bitsPerPixel = bitsPerPixelForFormat(frames.front().colorFormat);
        fileHeader.bitsPerColor = frameHasColorMap ? bitsPerPixelForFormat(frames.front().colorMapFormat) : 0;
        fileHeader.colorMapEntries = frameHasColorMap ? frames.front().colorMap.size() : 0;
        fileHeader.maxMemoryNeeded = maxMemoryNeeded;
        os.write(reinterpret_cast<const char *>(&fileHeader), sizeof(fileHeader));
        return os;
    }

}