#include "lz4.h"

#include "exception.h"
#include "if/lz4_constants.h"

#include <map>

// #define DEBUG_TOKENS
#ifdef DEBUG_TOKENS
#include <iostream>
#endif

// #define TEST_ROUNDTRIP

namespace Compression
{

    struct MatchInfo
    {
        int32_t distance = 0;
        int32_t length = 0;
    };

    auto findBestMatch(const std::vector<uint8_t> &src, const std::multimap<uint32_t, int32_t> &hashPositions, const int32_t srcPosition, const bool vramCompatible) -> MatchInfo
    {
        MatchInfo bestMatch = {0, 0};
        // get hash from srcPosition + MinLength
        const uint32_t srcPositionHash = *reinterpret_cast<const uint32_t *>(src.data() + srcPosition);
        // find all possible matches from hash table and exit if we have none
        const auto possibleMatches = hashPositions.equal_range(srcPositionHash);
        if (possibleMatches.first == hashPositions.cend())
        {
            return bestMatch;
        }
        // now find actual matches
        const int32_t MaxMatchLength = srcPosition + Lz4Constants::MAX_MATCH_LENGTH >= src.size() ? src.size() - srcPosition - 1 : Lz4Constants::MAX_MATCH_LENGTH;
        auto possibleMatchIt = possibleMatches.first;
        while (possibleMatchIt != possibleMatches.second)
        {
            // get position of possible match found in src and make sure it is towards front of buffer
            const auto matchStartPosition = possibleMatchIt->second;
            // calculate match distance and make sure it stays below the max. allowed distance
            const int32_t distance = srcPosition - matchStartPosition;
            // if we want to be VRAM-compatible, skip matches with a distance of 1
            if (vramCompatible && (distance == 1))
            {
                continue;
            }
            if (matchStartPosition < srcPosition && distance <= Lz4Constants::MAX_MATCH_DISTANCE)
            {
                // if we currently have no best match, store this one
                if (bestMatch.distance == 0 && bestMatch.length == 0)
                {
                    bestMatch = {distance, Lz4Constants::MIN_MATCH_LENGTH};
                }
                // we already know the match length is >= Lz4Constants::MIN_MATCH_LENGTH, so start with Lz4Constants::MIN_MATCH_LENGTH + 1
                for (int32_t matchLength = (Lz4Constants::MIN_MATCH_LENGTH + 1); matchLength <= MaxMatchLength; ++matchLength)
                {
                    // make sure we have enough bytes for a match of matchLength
                    if ((matchStartPosition + matchLength) >= src.size())
                    {
                        // we can't get better as don't have more bytes (should never happen)
                        return bestMatch;
                    }
                    // match byte at match position toward end of buffer and break if it does not match anymore
                    if (src[matchStartPosition + matchLength - 1] != src[srcPosition + matchLength - 1])
                    {
                        break;
                    }
                    // check if we've found a better match
                    if (matchLength > bestMatch.length)
                    {
                        bestMatch.distance = distance;
                        bestMatch.length = matchLength;
                        // check if we could improve or can return now
                        if (matchLength >= Lz4Constants::MAX_MATCH_LENGTH)
                        {
                            return bestMatch;
                        }
                    }
                }
            }
            ++possibleMatchIt;
        }
        return bestMatch;
    }

    auto extraLengthBytesNeeded(int32_t length) -> uint32_t
    {
        uint32_t extraBytesNeeded = 0;
        // 15 marks the start of a new byte
        length -= 15;
        while (length >= 0)
        {
            length -= 255;
            extraBytesNeeded++;
        }
        return extraBytesNeeded;
    }

    struct TokenInfo
    {
        std::vector<uint8_t> literals;
        uint32_t matchLength = 0;
        uint32_t matchOffset = 0;
    };

    auto flushToken(std::vector<uint8_t> &dst, const TokenInfo &token) -> void
    {
        uint8_t tokenByte = 0;
        const auto tokenOffset = dst.size();
        dst.push_back(0);
        // check if our token has literal bytes
        if (!token.literals.empty())
        {
            // store literal length
            int32_t storedLiteralLength = token.literals.size();
#ifdef DEBUG_TOKENS
            std::cout << "L(" << storedLiteralLength << "): \"" << std::string(reinterpret_cast<const char *>(token.literals.data()), token.literals.size()) << "\"";
            if (token.matchLength > 0)
            {
                std::cout << ", ";
            }
            else
            {
                std::cout << std::endl;
            }
#endif
            if (storedLiteralLength < 15)
            {
                // store literal length only in token byte
                tokenByte |= storedLiteralLength << Lz4Constants::LITERAL_LENGTH_SHIFT;
            }
            else
            {
                // store extra literal length bytes after token
                tokenByte |= 15 << 4;
                storedLiteralLength -= 15;
                while (storedLiteralLength >= 0)
                {
                    dst.push_back(storedLiteralLength >= 255 ? 255 : storedLiteralLength);
                    storedLiteralLength -= 255;
                }
            }
            // store literals
            std::copy(token.literals.cbegin(), token.literals.cend(), std::back_inserter(dst));
        }
        // check if if token has match
        if (token.matchLength > 0)
        {
#ifdef DEBUG_TOKENS
            std::cout << "M(" << token.matchOffset << "," << token.matchLength << ")" << std::endl;
#endif
            // store match offset
            REQUIRE(token.matchOffset > 0 && token.matchOffset <= Lz4Constants::MAX_MATCH_DISTANCE, std::runtime_error, "Match offset out of range [1,65535]");
            dst.push_back((token.matchOffset >> 8) & 0xFF);
            dst.push_back(token.matchOffset & 0xFF);
            // store literal length
            REQUIRE(token.matchLength >= Lz4Constants::MIN_MATCH_LENGTH, std::runtime_error, "Match length too small");
            int32_t storedMatchLength = token.matchLength - (Lz4Constants::MIN_MATCH_LENGTH - 1);
            if (storedMatchLength < 15)
            {
                // store match length only in token byte
                tokenByte |= storedMatchLength;
            }
            else
            {
                // store extra match length bytes after token
                tokenByte |= 15;
                storedMatchLength -= 15;
                while (storedMatchLength >= 0)
                {
                    dst.push_back(storedMatchLength >= 255 ? 255 : storedMatchLength);
                    storedMatchLength -= 255;
                }
            }
        }
        dst[tokenOffset] = tokenByte;
    }

    auto encodeLZ4_40(const std::vector<uint8_t> &src, bool vramCompatible) -> std::vector<uint8_t>
    {
        REQUIRE(!src.empty(), std::runtime_error, "Data too small");
        REQUIRE(src.size() < 2 ^ 24, std::runtime_error, "Data too big");
        const auto srcSize = src.size();
        // store uncompressed size and LZ4 marker flag at start of destination
        std::vector<uint8_t> dst(4, 0);
        *reinterpret_cast<uint32_t *>(dst.data()) = (src.size() << 8) | Lz4Constants::TYPE_MARKER;
        // build minmatch hash table for input data. it maps a hash (first Lz4Constants::MIN_MATCH_LENGTH bytes of data) to its position(s)
        std::multimap<uint32_t, int32_t> hashPositions;
        for (int32_t srcPosition = 0; srcPosition < static_cast<int32_t>(src.size() - Lz4Constants::MIN_MATCH_LENGTH); ++srcPosition)
        {
            const uint32_t hash = *reinterpret_cast<const uint32_t *>(src.data() + srcPosition);
            hashPositions.insert({hash, srcPosition});
        }
        // build match information for every byte except the last 4
        std::map<uint32_t, MatchInfo> matches;
#pragma omp parallel for
        for (int srcPosition = 0; srcPosition < static_cast<int>(src.size() - Lz4Constants::MIN_MATCH_LENGTH); ++srcPosition)
        {
            auto match = findBestMatch(src, hashPositions, srcPosition, vramCompatible);
            if (match.length >= Lz4Constants::MIN_MATCH_LENGTH)
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
            auto prevMatch = matches.cend();
            for (int32_t srcPosition = src.size() - 1; srcPosition > matches.cbegin()->first; --srcPosition)
            {
                auto matchIt = matches.find(srcPosition);
                if (matchIt != matches.cend())
                {
                    // here we are at a match. calculate cost
                    // a match stores as one byte, plus two bytes offset, plus one byte for match length > 15, two over 270, ...
                    auto bestLength = matchIt->second.length;
                    auto currMatchLength = bestLength;
                    auto bestCost = 3 + extraLengthBytesNeeded(currMatchLength - Lz4Constants::MIN_MATCH_LENGTH) + currentCost;
                    while (currMatchLength >= Lz4Constants::MIN_MATCH_LENGTH)
                    {
                        // cost of this match is the cost of the match plus the cost of the rest of the new encoding
                        // which is stored at cost[match_position + match_length]
                        const auto matchCost = 3 + extraLengthBytesNeeded(currMatchLength - Lz4Constants::MIN_MATCH_LENGTH) + cost[srcPosition + currMatchLength];
                        if (bestCost > matchCost)
                        {
                            bestCost = matchCost;
                            bestLength = currMatchLength;
                            matchIt->second.length = currMatchLength;
                        }
                        --currMatchLength;
                    }
                    // we need to find the length of the current literal run to determine how many length bytes we need
                    // a literal stores a one byte, plus one byte for literal length > 15, two over 270, ...
                    const uint32_t extraLiteralCost = matchIt != matches.cend() ? extraLengthBytesNeeded(matchIt->first - srcPosition - 1) : 0;
                    const auto literalCost = 1 + extraLiteralCost + cost[srcPosition + 1];
                    // check if storing literals is cheaper than the best current match
                    if (literalCost < bestCost)
                    {
                        bestCost = literalCost;
                        bestLength = 1;
                        // remove match from list. it is not needed anymore
                        matches.erase(srcPosition);
                    }
                    else
                    {
                        // store current match
                        prevMatch = matchIt;
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
        TokenInfo currentToken;
        std::size_t srcPosition = 0;
        while (srcPosition < src.size())
        {
            // check if the current byte will be a match
            if (auto matchIt = matches.find(srcPosition); matchIt != matches.cend())
            {
                // store the match in the current (or new) token
                currentToken.matchLength = matchIt->second.length;
                currentToken.matchOffset = matchIt->second.distance;
                // skip match length bytes in source
                srcPosition += matchIt->second.length;
                // flush the token
                flushToken(dst, currentToken);
                currentToken = TokenInfo();
            }
            else
            {
                // no matches found, store literal
                currentToken.literals.push_back(src[srcPosition++]);
            }
        }
        // if we still have a token pending, flush it
        if (!currentToken.literals.empty() || currentToken.matchLength > 0)
        {
            flushToken(dst, currentToken);
        }
        // resize to multiple of 4
        while ((dst.size() % 4) != 0)
        {
            dst.push_back(0);
        }
#ifdef TEST_ROUNDTRIP
        const auto result = decodeLZ4_40(dst, vramCompatible);
        REQUIRE(src == result, std::runtime_error, "Compression roundtrip failed");
#endif
        return dst;
    }

    auto decodeLZ4_40(const std::vector<uint8_t> &src, bool vramCompatible) -> std::vector<uint8_t>
    {
        REQUIRE(src.size() > 4, std::runtime_error, "Data too small");
        uint32_t header = *reinterpret_cast<const uint32_t *>(src.data());
        REQUIRE((header & 0xFF) == Lz4Constants::TYPE_MARKER, std::runtime_error, "Compression type not LZ4 (" << uint32_t(Lz4Constants::TYPE_MARKER) << ")");
        const uint32_t uncompressedSize = (header >> 8);
        REQUIRE(uncompressedSize > 0, std::runtime_error, "Bad uncompressed size");
        std::vector<uint8_t> dst;
        dst.reserve(uncompressedSize);
        // skip header in source data
        auto srcIt = std::next(src.cbegin(), 4);
        // decompress data
        while (dst.size() < uncompressedSize)
        {
            // read token
            uint8_t token = *srcIt++;
            uint32_t literalLength = (token >> Lz4Constants::LITERAL_LENGTH_SHIFT) & Lz4Constants::LENGTH_MASK;
            uint32_t matchLength = token & Lz4Constants::LENGTH_MASK;
            if (literalLength > 0)
            {
                // read extra literal length
                if (literalLength == 15)
                {
                    uint8_t extraLength = 0;
                    do
                    {
                        extraLength = *srcIt++;
                        literalLength += extraLength;
                    } while (extraLength == 255);
                }
                // copy literals following the length
                auto literalsStart = srcIt;
                auto literalsEnd = std::next(srcIt, literalLength);
#ifdef DEBUG_TOKENS
                std::cout << "L(" << literalLength << ")";
                if (matchLength > 0)
                {
                    std::cout << ", ";
                }
                else
                {
                    std::cout << std::endl;
                }
#endif
                std::copy(literalsStart, literalsEnd, std::back_inserter(dst));
                srcIt = literalsEnd;
            }
            if (matchLength > 0)
            {
                // read match offset
                int32_t matchOffset = (uint32_t(*srcIt++) << 8);
                matchOffset |= uint32_t(*srcIt++);
                REQUIRE(matchOffset > 0, std::runtime_error, "Zero match offset");
                REQUIRE(matchOffset <= dst.size(), std::runtime_error, "Match offset past end of data");
                // read extra match length
                if (matchLength == 15)
                {
                    uint8_t extraLength = 0;
                    do
                    {
                        extraLength = *srcIt++;
                        matchLength += extraLength;
                    } while (extraLength == 255);
                }
                matchLength += (Lz4Constants::MIN_MATCH_LENGTH - 1);
#ifdef DEBUG_TOKENS
                std::cout << "M(" << matchOffset << "," << matchLength << ")" << std::endl;
#endif
                // copy match from current byte - matchOffset until current byte - matchOffset + matchLength
                int32_t matchStart = int32_t(dst.size()) - matchOffset;
                REQUIRE(matchStart >= 0, std::runtime_error, "Match start outside of data");
                int32_t matchEnd = matchStart + matchLength;
                // we have to allow overlapping copies. check if this copy overlaps
                if (matchEnd >= dst.size())
                {
                    // overlapping copy
                    for (int32_t i = matchStart; i < matchEnd; ++i)
                    {
                        dst.push_back(dst[i]);
                    }
                }
                else
                {
                    // standard copy
                    std::copy(std::next(dst.cbegin(), matchStart), std::next(dst.cbegin(), matchEnd), std::back_inserter(dst));
                }
            }
        }
        return dst;
    }
}
