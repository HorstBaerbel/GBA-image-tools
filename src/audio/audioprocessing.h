#pragma once

#include "audiostructs.h"
#include "processingtype.h"

namespace Audio
{
    class Processing
    {
    public:
        static auto prependProcessingInfo(const Frame &processedData, uint32_t originalSize, ProcessingType type, bool isFinal) -> Frame;
    };
}