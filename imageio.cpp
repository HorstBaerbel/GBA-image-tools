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
        // store compressed frame data size
        const uint32_t frameSize = frame.data.size();
        os.write(reinterpret_cast<const char *>(&frameSize), sizeof(frameSize));
        // write frame data
        os.write(reinterpret_cast<const char *>(frame.data.data()), frame.data.size());
        // check if we're using a color map and write that
        if (hasColorMap(frame) > 0)
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

    auto IO::writeFileHeader(std::ostream &os, const std::vector<Data> &frames, uint8_t fps) -> std::ostream &
    {
        REQUIRE((sizeof(FileHeader) & 3) == 0, std::runtime_error, "FileHeader size is not a multiple of 4");
        // check if we're using a color map and calculate color map size
        const bool frameHasColorMap = hasColorMap(frames.front());
        const auto colorMapEntries = frameHasColorMap ? frames.front().colorMap.size() : 0;
        const auto colorMapSize = bytesPerColorMapEntry(frames.front()) * colorMapEntries;
        // calculate compressed frame data size
        const auto dataSize = std::accumulate(frames.cbegin(), frames.cend(), 0U, [](const auto &v, const auto &f)
                                              { return v + f.data.size(); });
        // generate file header and store it
        FileHeader fileHeader;
        fileHeader.fileSize = sizeof(FileHeader) + sizeof(uint32_t) * frames.size() + dataSize + colorMapSize * frames.size();
        fileHeader.nrOfFrames = frames.size();
        fileHeader.fps = fps;
        fileHeader.width = frames.front().size.width();
        fileHeader.height = frames.front().size.height();
        fileHeader.bitsPerPixel = bitsPerPixelForFormat(frames.front().format);
        fileHeader.bitsPerColor = frameHasColorMap ? bitsPerPixelForFormat(frames.front().colorMapFormat) : 0;
        fileHeader.colorMapEntries = colorMapEntries;
        os.write(reinterpret_cast<const char *>(&fileHeader), sizeof(fileHeader));
        return os;
    }

}
