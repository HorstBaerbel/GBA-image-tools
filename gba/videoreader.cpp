#include "videoreader.h"

#include "memory.h"

namespace Video
{

    Info GetInfo(const uint8_t *data)
    {
        Info info;
        Memory::memcpy32(&info, data, sizeof(FileHeader));
        info.fileData = data;
        info.frameData = data + sizeof(FileHeader) + 4 + 4;
        return info;
    }

    Frame GetNextFrame(const Info &info, const Frame &previous)
    {
        Frame frame;
        if (previous.index == 0 || previous.index >= (info.nrOfFrames - 1))
        {
            // read first frame
            frame.index = 0;
            frame.chunkOffset = 0;
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
            frame.chunkOffset = previous.chunkOffset + sizeof(Frame::compressedSize) + sizeof(DataChunk) + previous.compressedSize + colorMapSize;
        }
        frame.compressedSize = *reinterpret_cast<const uint32_t *>(info.frameData + frame.chunkOffset);
        frame.colorMapOffset = info.colorMapEntries > 0 ? frame.chunkOffset + frame.compressedSize : 0;
        return frame;
    }

}