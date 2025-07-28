#include "audioprocessing.h"

#include "audiohelpers.h"
#include "exception.h"
#include "processing/datahelpers.h"

namespace Audio
{
    Frame Processing::prependProcessingInfo(const Frame &processedData, uint32_t originalSize, ProcessingType type, bool isFinal)
    {
        auto rawData = AudioHelpers::toRawData(processedData.data, processedData.info.channelFormat);
        REQUIRE(rawData.size() < (1 << 24), std::runtime_error, "Raw data size stored must be < 16MB");
        REQUIRE(static_cast<uint32_t>(type) <= 127, std::runtime_error, "Type value must be <= 127");
        const uint32_t sizeAndType = ((originalSize & 0xFFFFFF) << 8) | ((static_cast<uint32_t>(type) & 0x7F) | (isFinal ? static_cast<uint32_t>(ProcessingTypeFinal) : 0));
        auto result = processedData;
        result.data = DataHelpers::prependValue(rawData, sizeAndType);
        return result;
    }
}