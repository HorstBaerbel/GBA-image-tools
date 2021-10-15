#include "videoreader.h"

#include "memory.h"

namespace VideoReader
{

    FileHeader GetFileHeader(const uint8_t *data)
    {
        FileHeader header;
        Memory::memcpy32(&header, data, sizeof(header));
        return header;
    }

}