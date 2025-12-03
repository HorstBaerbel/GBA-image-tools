#include "lzss.h"

#include "exception.h"

#include <map>

namespace Compression
{

    constexpr uint32_t MIN_MATCH_LENGTH = 3;
    constexpr uint32_t MAX_MATCH_LENGTH = 18;      // We have max. 4 bits to encode match length [3,18]
    constexpr uint32_t MAX_MATCH_DISTANCE = 0xFFF; // We have max. 12 bits to encode match distance

    struct MatchInfo
    {
        int32_t distance = 0;
        int32_t length = 0;
    };

    template <int32_t MinLength, int32_t MaxLength>
    auto findBestMatch(const std::vector<uint8_t> &src, const int32_t startPosition, const bool vramCompatible) -> MatchInfo
    {
        MatchInfo bestMatch = {0, 0};
        for (int32_t matchLength = MinLength; matchLength <= MaxLength; ++matchLength)
        {
            // make sure we have enough bytes for a match of matchLength
            if ((startPosition + matchLength) >= src.size())
            {
                // we can't get better as don't have more bytes
                return bestMatch;
            }
            // start matching matchLength bytes backwards and move towards front of buffer
            for (auto matchPosition = startPosition - matchLength; matchPosition >= 0; --matchPosition)
            {
                // calculate match distance and make sure it stays below the max. allowed distance
                int32_t distance = startPosition - matchPosition;
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
                    if (src[matchPosition + i] != src[startPosition + i])
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
        // store uncompressed size and LZ10 marker flag at start of destination
        std::vector<uint8_t> dst(4, 0);
        *reinterpret_cast<uint32_t *>(dst.data()) = (src.size() << 8) | 0x10;
        // build match information for every byte
        std::map<uint32_t, MatchInfo> matches;
#pragma omp parallel for
        for (int srcPosition = 0; srcPosition < static_cast<int>(src.size()); ++srcPosition)
        {
            auto match = findBestMatch<MIN_MATCH_LENGTH, MAX_MATCH_LENGTH>(src, srcPosition, vramCompatible);
            if (match.length >= MIN_MATCH_LENGTH)
#pragma omp critical
            {
                matches[srcPosition] = match;
            }
        }
        // if we haven't found any matches, no need to check them
        if (!matches.empty())
        {
            // optimize match cost
            std::vector<uint32_t> cost(src.size(), 0);
            uint32_t currentCost = 0;
            // iterate through the matches in reverse until the first match
            for (uint32_t srcPosition = src.size() - 1; srcPosition > matches.cbegin()->first; --srcPosition)
            {
                auto matchIt = matches.find(srcPosition);
                if (matchIt != matches.cend())
                {
                    // here we are at a match. calculate cost
                    auto bestCost = 2 + currentCost;
                    auto bestLength = matchIt->second.length;
                    auto currMatchLength = bestLength;
                    while (currMatchLength >= MIN_MATCH_LENGTH)
                    {
                        // cost of this match is the cost of the match plus the cost of the rest of the new encoding
                        // which is stored at cost[match_position + match_length]
                        const auto matchCost = 2 + cost[srcPosition + currMatchLength];
                        if (bestCost > matchCost)
                        {
                            bestCost = matchCost;
                            bestLength = currMatchLength;
                            matchIt->second.length = currMatchLength;
                        }
                        --currMatchLength;
                    }
                    // check if storing literals is cheaper than the best current match
                    const auto literalCost = 1 + cost[srcPosition + 1];
                    if (literalCost < bestCost)
                    {
                        bestCost = literalCost;
                        bestLength = 1;
                        // remove match from list. it is not needed anymore
                        matches.erase(srcPosition);
                    }
                    // store current match cost
                    cost[srcPosition] = bestCost;
                    currentCost = bestCost;
                }
                else
                {
                    // here we have a literal
                    cost[srcPosition] = ++currentCost;
                }
            }
        }
        // compress source by iterating through matches
        std::size_t dstFlagPosition = 0;
        int32_t flagBitIndex = 7;
        std::size_t srcPosition = 0;
        while (srcPosition < src.size())
        {
            if (flagBitIndex++ == 7)
            {
                // insert flag byte for later and store its location
                dstFlagPosition = dst.size();
                dst.push_back(0);
                flagBitIndex = 0;
            }
            // check if the current byte will be a match
            if (auto matchIt = matches.find(srcPosition); matchIt != matches.cend())
            {
                // yes. compress the match
                auto storedMatchLength = matchIt->second.length - MIN_MATCH_LENGTH;
                REQUIRE(storedMatchLength >= 0 && storedMatchLength < 16, std::runtime_error, "Stored match length out of range [0,15]");
                auto storedDistance = matchIt->second.distance - 1;
                REQUIRE(storedDistance >= 0 && storedDistance <= MAX_MATCH_DISTANCE, std::runtime_error, "Stored match distance out of range [0,0xFFF]");
                // store 4 bits of match length and 12 bits of match distance
                dst.push_back((storedMatchLength << 4) | (storedDistance >> 8));
                dst.push_back(storedDistance & 0xFF);
                // store "compressed" flag
                dst[dstFlagPosition] |= (0x80 >> flagBitIndex);
                // skip match length bytes in source
                srcPosition += matchIt->second.length;
            }
            else
            {
                // no matches found, store verbatim byte and "not compressed" zero flag
                dst.push_back(src[srcPosition++]);
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
        REQUIRE((header & 0xFF) == 0x10, std::runtime_error, "Compression type not LZSS (10h)");
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
