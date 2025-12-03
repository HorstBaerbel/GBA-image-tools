#include "rans.h"

#include "exception.h"
#include "math/histogram.h"

#include <map>
#include <cmath>
#include <cstring>

namespace Compression
{

    auto encodeRANS(const std::vector<uint8_t> &src) -> std::vector<uint8_t>
    {
        REQUIRE(!src.empty(), std::runtime_error, "Data too small");
        REQUIRE(src.size() < 2 ^ 24, std::runtime_error, "Data too big");
        const auto srcSize = src.size();
        // store uncompressed size and rANS type flag at start of destination
        std::vector<uint8_t> dst(4, 0);
        *reinterpret_cast<uint32_t *>(dst.data()) = (src.size() << 8) | 0x40;
        // build and sum histogram
        std::vector<uint32_t> histogram(RANS_ALPHABET_SIZE, 0);
        for (char c : src)
        {
            histogram[(uint8_t)c]++;
        }
        const double total = src.size() == 0 ? 1 : src.size();
        // scale histogram to 8-bit freqs and build rANS tables
        std::vector<uint8_t> counts(RANS_ALPHABET_SIZE, 0);
        std::vector<uint32_t> starts(RANS_ALPHABET_SIZE, 0);
        uint32_t total8 = 0;
        for (uint32_t i = 0; i < RANS_ALPHABET_SIZE; i++)
        {
            double count = histogram.at(i);
            uint32_t frequency = 0;
            if (count > 0)
            {
                frequency = static_cast<uint32_t>(((count * double(RANS_L)) / total) + 0.5);
                frequency = frequency == 0 ? 1 : frequency;
                counts[i] = static_cast<uint8_t>(frequency);
                total8 += frequency;
            }
        }
        // check if we have inconsistent counts due to rounding errors
        if (total8 != RANS_L)
        {
            // TODO
        }
        // calculate symbol starts
        uint32_t currentStart = 0;
        for (uint32_t i = 0; i < RANS_ALPHABET_SIZE; i++)
        {
            starts[i] = currentStart;
            currentStart += counts[i];
        }
        // store frequency table in header
        std::copy(counts.cbegin(), counts.cend(), std::back_inserter(dst));
        // reserve worst case size temporary vector (input size + small margin)
        std::vector<uint8_t> temp;
        temp.reserve(src.size() + 8);
        // encode backwards
        uint32_t x = RANS_M;
        auto srcIt = src.crbegin();
        while (srcIt != src.crend())
        {
            auto symbol = *srcIt++;
            uint32_t count = counts[symbol];
            uint32_t start = starts[symbol];
            while (x >= (RANS_M / count) * count)
            {
                temp.push_back(static_cast<uint8_t>(x));
                x >>= 8;
            }
            x = (x / count << RANS_L_BITS) + start + (x % count);
        }
        // flush final state
        while (x > 0)
        {
            temp.push_back(static_cast<uint8_t>(x));
            x >>= 8;
        }
        // copy to end of destination in reverse
        std::copy(temp.crbegin(), temp.crend(), std::back_inserter(dst));
        // resize to multiple of 4
        while ((dst.size() % 4) != 0)
        {
            dst.push_back(0);
        }
        return dst;
    }

    auto decodeRANS(const std::vector<uint8_t> &src) -> std::vector<uint8_t>
    {
        REQUIRE(src.size() >= (4 + 256 + 4), std::runtime_error, "Data too small");
        uint32_t header = *reinterpret_cast<const uint32_t *>(src.data());
        REQUIRE((header & 0xFF) == 0x40, std::runtime_error, "Compression type not rANS (40h)");
        const uint32_t uncompressedSize = (header >> 8);
        REQUIRE(uncompressedSize > 0, std::runtime_error, "Bad uncompressed size");
        std::vector<uint8_t> dst;
        dst.reserve(uncompressedSize);
        // read frequency table
        std::vector<uint8_t> counts(RANS_ALPHABET_SIZE, 0);
        std::copy(std::next(src.cbegin(), 4), std::next(src.cbegin(), 4 + 256), counts.begin());
        // build decode tables
        uint8_t symbols[RANS_L];         // The decoded symbol (0-255)
        uint8_t newIndexOffsets[RANS_L]; // The offset for the new state calculation: (x_tilde - start_S)
        // We map L slots (0 to 255) to the symbol and its properties.
        uint32_t x_tilde = 0;
        for (size_t i = 0; i < RANS_ALPHABET_SIZE; ++i)
        {
            uint32_t count = counts[i];
            uint8_t symbol = (uint8_t)i;
            // Map each of the 'count' slots for the current symbol
            for (uint32_t j = 0; j < count; ++j)
            {
                REQUIRE(x_tilde < RANS_L, std::runtime_error, "L size mismatch: Total counts exceed L=256");
                symbols[x_tilde] = symbol;
                // store the New Index Offset: (x_tilde - start)
                // this offset is the index j, i.e., the position of x_tilde within the symbol's range.
                // x_tilde = start_S + j  =>  j = x_tilde - start_S
                newIndexOffsets[x_tilde] = (uint8_t)j;
                x_tilde++;
            }
        }
        // skip header and frequency table in source data
        auto srcIt = std::next(src.cbegin(), 4 + 256);
        // read initial rANS state
        uint32_t x = 0;
        while (x < RANS_M)
        {
            if (srcIt == src.cend())
            {
                break;
            }
            x = (x << 8) | *srcIt++;
        }
        // decompress data
        while (dst.size() < uncompressedSize)
        {
            // find the symbol and its required offset
            uint32_t x_tilde = x & (RANS_L - 1);
            // look up the symbol (S) and the inner-symbol index (x_tilde - start)
            uint8_t symbol = symbols[x_tilde];
            dst.push_back(symbol);
            uint8_t offset = newIndexOffsets[x_tilde];
            // look up the count (c) separately
            uint32_t count = counts[symbol];
            // state update (decode)
            // x_old = c * floor(x / L) + (x_tilde - start)
            // where (x_tilde - start) is new index offset
            x = count * (x >> RANS_L_BITS) + offset;
            // renormalization (input)
            while (x < RANS_M)
            {
                if (srcIt == src.cend())
                {
                    x <<= 8;
                    break;
                }
                x = (x << 8) | *srcIt++;
            }
        }
        REQUIRE(dst.size() == uncompressedSize, std::runtime_error, "Bad data size after decoding");
        return dst;
    }
}