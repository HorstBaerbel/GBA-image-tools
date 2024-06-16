#include "lzss.h"

#include "exception.h"

#ifdef _MSC_VER
#include <process.h>
#else
#include <unistd.h>
#endif

#include <fstream>
#include <filesystem>

namespace Compression
{

    constexpr int32_t MIN_MATCH_LENGTH = 3;
    constexpr int32_t MAX_MATCH_LENGTH = 18; // We have max. 4 bits to encode match length [3,18]
    constexpr int32_t MAX_MATCH_DISTANCE = 0xFFF;

    auto findMatch(std::vector<uint8_t>::const_iterator startIt, std::vector<uint8_t>::const_iterator endIt, int32_t matchLength, bool vramCompatible) -> int32_t
    {
        // mkae sure we have enough bytes for a match of matchLength
        if (std::next(startIt, matchLength) >= endIt)
        {
            return 0;
        }
        // start matching matchLength bytes backwards and move to front of buffer
        for (auto matchIt = std::prev(startIt, matchLength); matchIt >= startIt; --matchIt)
        {
            int32_t distance = std::distance(startIt, matchIt);
            auto storedDistance = -distance - 1;
            if (storedDistance > MAX_MATCH_DISTANCE)
            {
                return 0;
            }
            // if we want to be VRAM-compatible, skip matches with a distance of 1
            if (vramCompatible && (distance == 1))
            {
                continue;
            }
            bool match = true;
            for (int32_t i = 0; i < matchLength; ++i)
            {
                if (matchIt[i] != startIt[i])
                {
                    match = false;
                    break;
                }
            }
            if (match)
            {
                return distance;
            }
        }
        return 0;
    }

    auto encodeLZ10(const std::vector<uint8_t> &src, bool vramCompatible) -> std::vector<uint8_t>
    {
        REQUIRE(!src.empty(), std::runtime_error, "Data too small");
        REQUIRE(src.size() < 2 ^ 24, std::runtime_error, "Data too big");
        const auto srcSize = src.size();
        auto srcIt = src.cbegin();
        // leave space for header at start of destination
        std::vector<uint8_t> dst(4, 0);
        auto dstLastFlags = dst.back();
        int32_t flagBitsCount = 0;
        uint8_t flags = 0;

        while (srcIt < src.cend())
        {
            if (flagBitsCount == 0)
            {
                // insert flag byte for later and store its location
                dst.push_back(0);
                dstLastFlags = dst.back();
            }
            // try to find minimum match
            int32_t matchDistance = findMatch(srcIt, src.cend(), MIN_MATCH_LENGTH, vramCompatible);
            if (matchDistance != 0)
            {
                // found minimal match, try to find better ones
                int32_t matchLength = 3;
                // start at MIN_MATCH_LENGTH + 1, as we've tested MIN_MATCH_LENGTH already
                for (int32_t length = MIN_MATCH_LENGTH + 1; length <= MAX_MATCH_LENGTH; ++length)
                {
                    auto distance = findMatch(srcIt, src.cend(), length, vramCompatible);
                    if (distance == 0)
                    {
                        // there can be no better matches at this point
                        break;
                    }
                    // store best match
                    matchLength = length;
                    matchDistance = distance;
                }
                srcIt += matchLength;
                auto storedMatchLength = matchLength - MIN_MATCH_LENGTH;
                REQUIRE(storedMatchLength >= 0 && storedMatchLength < 16, std::runtime_error, "Match length out of range [0,15]");
                auto storedDistance = -matchDistance - 1;
                REQUIRE(storedDistance >= 0 && storedDistance <= MAX_MATCH_DISTANCE, std::runtime_error, "Match distance out of range[0,0xFFF]");
                // store 4 bits of match length and 12 bits of match distance
                dst.push_back((storedMatchLength << 4) | ((storedDistance & 0xF00) >> 8));
                dst.push_back(storedDistance & 0xFF);
                flags |= 0x80 >> flagBitsCount;
            }
            else
            {
                // no matches found, store verbatim byte, store no flags
                dst.push_back(*srcIt++);
            }
            flagBitsCount++;
            // store current flags, if we have one whole flag byte, or are at the end of the data
            if (flagBitsCount == 8 || srcIt >= src.cend())
            {
                flagBitsCount = 0;
                dstLastFlags = flags;
                flags = 0;
            }
        }
        // resize to multiple of 4
        while ((dst.size() % 4) != 0)
        {
            dst.push_back(0);
        }
        // store uncompressed size and LZ10 marker flag
        *reinterpret_cast<uint32_t *>(dst.data()) = (src.size() << 8) | 0x10;
        return dst;
    }

    auto decodeLZ10(const std::vector<uint8_t> &src, bool vramCompatible) -> std::vector<uint8_t>
    {
        REQUIRE(src.size() > 4, std::runtime_error, "Data too small");
        uint32_t header = *reinterpret_cast<const uint32_t *>(src.data());
        REQUIRE((header & 0xFF) == 0x10, std::runtime_error, "Data not variant 10h");
        const uint32_t uncompressedSize = (header >> 8);
        REQUIRE(uncompressedSize > 0, std::runtime_error, "Bad uncompressed size");
        std::vector<uint8_t> dst;
        dst.reserve(uncompressedSize);
        auto srcIt = std::next(src.cbegin(), 4);
        while (dst.size() < uncompressedSize)
        {
            // read flags for next 8 tokens
            uint8_t flags = *srcIt++;
            for (int32_t i = 0; i < 8 && dst.size() < uncompressedSize; ++i)
            {
                // check if next token is match or verbatim byte
                if ((flags & (0x80 >> i)) != 0)
                {
                    int32_t matchLenght = (*srcIt >> 4) + MIN_MATCH_LENGTH;
                    uint32_t matchDistance = (*srcIt++ & 0xF) << 4;
                    matchDistance |= *srcIt++;
                    auto copyStartIt = std::prev(dst.cend(), matchDistance + 1);
                    std::copy(copyStartIt, std::prev(dst.cend()), std::back_inserter(dst));
                }
                else
                {
                    dst.push_back(*srcIt++);
                }
            }
        }
        return dst;
    }
}
