#include "videoreader.h"

#include "memory.h"

namespace Video
{

    Info GetInfo(const uint32_t *data)
    {
        static_assert(sizeof(FileHeader) % 4 == 0);
        Info info;
        Memory::memcpy32(&info, data, sizeof(FileHeader));
        info.fileData = data;
        info.frameData = data + sizeof(FileHeader) / 4;
        return info;
    }

    Frame GetNextFrame(const Info &info, const Frame &previous)
    {
        Frame frame;
        if (previous.index == 0 || previous.index >= (info.nrOfFrames - 1))
        {
            // read first frame
            frame.index = 0;
            frame.chunkOffset = sizeof(Frame::compressedSize);
        }
        else
        {
            frame.index = previous.index + 1;
            auto colorMapSize = info.colorMapEntries;
            switch (info.bitsInColorMap)
            {
            case 15:
            case 16:
                colorMapSize *= 2;
                break;
            case 24:
                colorMapSize *= 3;
                break;
            default:
                // TODO: What?
                break;
            }
            frame.chunkOffset = previous.chunkOffset + sizeof(DataChunk) + previous.compressedSize + colorMapSize + sizeof(Frame::compressedSize);
        }
        frame.compressedSize = *(info.frameData + frame.chunkOffset / 4);
        frame.colorMapOffset = info.colorMapEntries > 0 ? frame.chunkOffset + frame.compressedSize : 0;
        return frame;
    }

}