#include "vid2hio.h"

#include "memory/memory.h"

namespace IO::Vid2h
{

    Info GetInfo(const uint32_t *data)
    {
        static_assert(sizeof(FileHeader) % 4 == 0);
        Info info;
        Memory::memcpy32(&info, data, sizeof(FileHeader) / 4);
        info.fileData = data;
        info.nrOfFrames = info.videoNrOfFrames + info.videoNrOfColorMapFrames + info.audioNrOfFrames;
        info.imageSize = info.videoWidth * info.videoHeight;
        switch (info.videoBitsPerColor)
        {
        case 1:
            info.imageSize = (info.imageSize + 7) / 8;
            break;
        case 2:
            info.imageSize = (info.imageSize + 3) / 4;
            break;
        case 4:
            info.imageSize = (info.imageSize + 1) / 2;
            break;
        case 8:
            info.imageSize *= 1;
            break;
        case 15:
        case 16:
            info.imageSize *= 2;
            break;
        case 24:
            info.imageSize *= 3;
            break;
        default:
            // TODO: What?
            break;
        }
        info.colorMapSize = info.videoColorMapEntries;
        switch (info.videoBitsPerColor)
        {
        case 15:
        case 16:
            info.colorMapSize *= 2;
            break;
        case 24:
            info.colorMapSize *= 3;
            break;
        default:
            // TODO: What?
            break;
        }
        return info;
    }

    auto HasMoreFrames(const Info &info, const Frame &previous) -> bool
    {
        return previous.index < static_cast<int32_t>(info.nrOfFrames - 1);
    }

    Frame GetNextFrame(const Info &info, const Frame &previous)
    {
        static_assert(sizeof(FrameHeader) % 4 == 0);
        Frame frame;
        const uint32_t *frameStart = nullptr;
        if (previous.index < 0 || previous.index >= static_cast<int32_t>(info.nrOfFrames - 1))
        {
            // read first frame
            frame.index = 0;
            frameStart = info.fileData + sizeof(FileHeader) / 4;
        }
        else
        {
            // read next frame
            frame.index = previous.index + 1;
            frameStart = previous.header + sizeof(FrameHeader) / 4 + previous.dataSize / 4;
        }
        auto frameHeader = reinterpret_cast<const FrameHeader *>(frameStart);
        frame.dataType = frameHeader->dataType;
        frame.dataSize = frameHeader->dataSize;
        frame.header = frameStart;
        frame.data = frameStart + sizeof(FrameHeader) / 4;
        return frame;
    }
}