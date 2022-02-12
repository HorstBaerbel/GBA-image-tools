#include "codec_rle.h"

#include "colorhelpers.h"
#include "exception.h"

auto RLE::compressRLE(const std::vector<uint8_t> &data, bool /*vramCompatible*/) -> std::vector<uint8_t>
{
    constexpr int32_t MinRepeatLength = 3;
    constexpr int32_t MaxRepeatLength = ((1 << 7) - 1) + MinRepeatLength;
    constexpr int32_t MaxCopyLength = ((1 << 7) - 1) + 1;
    std::vector<uint8_t> result;
    auto runStart = data.cbegin();
    auto runEnd = std::next(data.cbegin());
    std::size_t runLength = 1;
    bool isRepetition = false;
    // std::cout << data.size() << std::endl;
    while (runEnd < data.cend())
    {
        // find repetition
        while (runEnd < data.cend() && *runStart == *runEnd && runLength < MaxRepeatLength)
        {
            isRepetition = true;
            runEnd = std::next(runEnd);
            runLength++;
        }
        if (isRepetition)
        {
            // a repetition was found and has ended
            if (runLength >= MinRepeatLength)
            {
                // store repetition if longer than min repetition length
                result.push_back((1 << 7) + (runLength - MinRepeatLength));
                result.push_back(*runStart);
                runStart = std::next(runStart, runLength);
                runEnd = runStart;
                runLength = 1;
            }
            // else, treat it as a verbatim copy run
            isRepetition = false;
        }
        if (runLength >= MaxCopyLength)
        {
            // the max copy length has been reached. store verbatim copy
            result.push_back(runLength - 1);
            std::copy(runStart, runEnd, std::back_inserter(result));
            runStart = std::next(runStart, runLength);
            runEnd = runStart;
            runLength = 1;
        }
        runEnd = std::next(runEnd);
    }
    // we might have some leftover verbatim bytes here. store them
    if (runLength > 1)
    {
        result.push_back(runLength - 1);
        std::copy(runStart, runEnd, std::back_inserter(result));
    }
    return result;
}
