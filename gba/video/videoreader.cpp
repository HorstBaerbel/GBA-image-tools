#include "videoreader.h"

#include "memory/memory.h"

namespace Video
{

    Info GetInfo(const uint32_t *data)
    {
        static_assert(sizeof(FileHeader) % 4 == 0);
        Info info;
        Memory::memcpy32(&info, data, sizeof(FileHeader) / 4);
        info.fileData = data;
        info.colorMapSize = info.colorMapEntries;
        switch (info.bitsInColorMap)
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

    Frame GetNextFrame(const Info &info, const Frame &previous)
    {
        static_assert(sizeof(Frame::compressedSize) % 4 == 0);
        static_assert(sizeof(DataChunk) % 4 == 0);
        Frame frame;
        if (previous.index < 0 || previous.index >= static_cast<int32_t>(info.nrOfFrames - 1))
        {
            // read first frame
            frame.index = 0;
            frame.data = info.fileData + sizeof(FileHeader) / 4;
        }
        else
        {
            frame.index = previous.index + 1;
            frame.data = previous.data + (sizeof(Frame::compressedSize) + previous.compressedSize + info.colorMapSize) / 4;
        }
        frame.compressedSize = *frame.data;
        frame.colorMapOffset = info.colorMapEntries > 0 ? (sizeof(Frame::compressedSize) + frame.compressedSize) : 0;
        return frame;
    }

}