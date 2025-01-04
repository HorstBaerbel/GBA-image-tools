#include "lzss.h"

#include "exception.h"

#ifdef _MSC_VER
#include <process.h>
#else
#include <unistd.h>
#endif

#include <filesystem>
#include <fstream>

namespace Compression
{

    constexpr uint32_t MIN_MATCH_LENGTH = 3;
    constexpr uint32_t MAX_MATCH_LENGTH = 18;      // We have max. 4 bits to encode match length [3,18]
    constexpr uint32_t MAX_MATCH_DISTANCE = 0xFFF; // We have max. 12 bits to encode match distance

    auto findBestMatch(std::vector<uint8_t>::const_iterator beginIt, std::vector<uint8_t>::const_iterator startIt, std::vector<uint8_t>::const_iterator endIt, int32_t minLength, int32_t maxLength, bool vramCompatible) -> std::pair<int32_t, int32_t>
    {
        std::pair<int32_t, int32_t> bestMatch = {0, 0};
        for (int32_t matchLength = minLength; matchLength <= maxLength; ++matchLength)
        {
            // make sure we have enough bytes for a match of matchLength
            if (std::next(startIt, matchLength) >= endIt)
            {
                // we can't get better as don't have more bytes
                return bestMatch;
            }
            // start matching matchLength bytes backwards and move towards front of buffer
            for (auto matchIt = std::prev(startIt, matchLength); matchIt >= beginIt; --matchIt)
            {
                // calculate match distance and make sure it stays below the max. allowed distance
                int32_t distance = std::distance(matchIt, startIt);
                if ((distance - 1) > MAX_MATCH_DISTANCE)
                {
                    break;
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
                    bestMatch = {distance, matchLength};
                    break;
                }
            }
        }
        return bestMatch;
    }

    auto encodeLZ10(const std::vector<uint8_t> &src, bool vramCompatible) -> std::vector<uint8_t>
    {
        REQUIRE(!src.empty(), std::runtime_error, "Data too small");
        REQUIRE(src.size() < 2 ^ 24, std::runtime_error, "Data too big");
        const auto srcSize = src.size();
        auto srcIt = src.cbegin();
        // // store uncompressed size and LZ10 marker flag at start of destination
        std::vector<uint8_t> dst(4, 0);
        *reinterpret_cast<uint32_t *>(dst.data()) = (src.size() << 8) | 0x10;
        // compress source
        std::size_t dstFlagPosition = 0;
        int32_t flagBitIndex = 7;
        while (srcIt < src.cend())
        {
            uint32_t distance = std::distance(src.cbegin(), srcIt);
            if (flagBitIndex++ == 7)
            {
                // insert flag byte for later and store its location
                dstFlagPosition = std::distance(dst.cbegin(), dst.cend());
                dst.push_back(0);
                flagBitIndex = 0;
            }
            // prime destination with one verbatim character
            if (srcIt == src.cbegin())
            {
                dst.push_back(*srcIt++);
                continue;
            }
            // try to find next match
            auto [matchDistance, matchLength] = findBestMatch(src.cbegin(), srcIt, src.cend(), MIN_MATCH_LENGTH, MAX_MATCH_LENGTH, vramCompatible);
            /*// The comment code is what GBALZSS does, but it fails to produce the same data. Go figure...
            if (matchLength >= MIN_MATCH_LENGTH)
            {
                // we've found a match. check how long the next match after this is
                auto [nextDistance, nextLength] = findBestMatch(src.cbegin(), std::next(srcIt, matchLength), src.cend(), MIN_MATCH_LENGTH, MAX_MATCH_LENGTH, vramCompatible);
                if (nextLength < MIN_MATCH_LENGTH)
                {
                    nextLength = 1;
                }
                // check how long the match after the next byte is
                auto [skipDistance, skipLength] = findBestMatch(src.cbegin(), std::next(srcIt, 1), src.cend(), MIN_MATCH_LENGTH, MAX_MATCH_LENGTH, vramCompatible);
                if (skipLength < MIN_MATCH_LENGTH)
                {
                    skipLength = 1;
                }
                // is it worth compressing this match?
                if ((matchLength + nextLength) <= (skipLength + 1))
                {
                    // not worth it. byte will be encoded verbatim
                    matchLength = 1;
                }
            }*/
            if (matchLength < MIN_MATCH_LENGTH)
            {
                // no matches found, store verbatim byte and "not compressed" zero flag
                dst.push_back(*srcIt++);
            }
            else
            {
                // yes. compress the match
                srcIt += matchLength;
                auto storedMatchLength = matchLength - MIN_MATCH_LENGTH;
                REQUIRE(storedMatchLength >= 0 && storedMatchLength < 16, std::runtime_error, "Match length out of range [0,15]");
                auto storedDistance = matchDistance - 1;
                REQUIRE(storedDistance >= 0 && storedDistance <= MAX_MATCH_DISTANCE, std::runtime_error, "Match distance out of range[0,0xFFF]");
                // store 4 bits of match length and 12 bits of match distance
                dst.push_back((storedMatchLength << 4) | (storedDistance >> 8));
                dst.push_back(storedDistance & 0xFF);
                // store "compressed" flag
                dst[dstFlagPosition] |= (0x80 >> flagBitIndex);
            }
        }
        // resize to multiple of 4
        while ((dst.size() % 4) != 0)
        {
            dst.push_back(0);
        }
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
        // skip header in source data
        auto srcIt = std::next(src.cbegin(), 4);
        // decompress data
        while (dst.size() < uncompressedSize)
        {
            // read flags for next 8 tokens
            uint8_t flags = *srcIt++;
            for (int32_t flagBitIndex = 0; flagBitIndex < 8 && dst.size() < uncompressedSize; ++flagBitIndex)
            {
                // check if next token is match or verbatim byte
                if ((flags & (0x80 >> flagBitIndex)) != 0)
                {
                    // copy data for match from decoded buffer
                    uint32_t matchLenght = (*srcIt >> 4) + MIN_MATCH_LENGTH;
                    uint32_t matchDistance = (*srcIt++ & 0xF) << 8;
                    matchDistance |= *srcIt++;
                    auto copyStartIt = std::prev(dst.cend(), matchDistance + 1);
                    // make sure to clamp copy size to not overrun buffer
                    auto copyLength = std::min(uncompressedSize - static_cast<uint32_t>(dst.size()), matchLenght);
                    auto copyEndIt = std::next(copyStartIt, copyLength);
                    std::copy(copyStartIt, copyEndIt, std::back_inserter(dst));
                }
                else
                {
                    // store verbatim byte
                    dst.push_back(*srcIt++);
                }
            }
        }
        return dst;
    }
}
