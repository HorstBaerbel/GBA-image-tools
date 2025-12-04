#include "rans.h"

#include "exception.h"
#include "math/histogram.h"

#include <map>
#include <cmath>
#include <cstring>

namespace Compression
{

    // Header encodings (one byte):
    // 2 bits of mode:
    // 0: Single symbol
    //    Symbol index stored
    // 1: 256 counts for all symbols
    //    Complete count table stored
    // All other encodings have 6 bits of minimum symbol count (subtracted from all counts)
    // 2: RLE with literal blocks
    //    Use the most significant bit (MSB) of the control byte as a flag:
    //    Flag 0 (Literal block): 0 + 6-bit count. The following count + 1 bytes are literal data (raw copies)
    //    Flag 1 (Run block): 1 + 3-bit count, 4-bit value. The value is repeated count + 1 times. if value == 15 an extra value byte follows

    static std::vector<uint8_t> calculateCounts(const std::vector<uint32_t> &histogram)
    {
        auto total = std::accumulate(histogram.cbegin(), histogram.cend(), uint32_t(0));
        REQUIRE(total >= 1, std::runtime_error, "Empty input for scaling");
        // build counts and fractions table
        std::vector<std::pair<uint8_t, double>> fractions; // stores (symbol, fraction above 8-bit count)
        fractions.reserve(RANS_ALPHABET_SIZE);
        uint32_t total8 = 0;
        std::vector<uint8_t> counts(RANS_ALPHABET_SIZE, 0);
        for (size_t i = 0; i < RANS_ALPHABET_SIZE; ++i)
        {
            // ignore synbols that do not occur in the data
            if (histogram[i] == 0)
            {
                continue;
            }
            // calculate floating-point count in [0, RANS_M]
            double countM = (histogram[i] * double(RANS_M)) / static_cast<double>(total);
            // calculate 8-bit count and clamp to [0, RANS_M - 1]
            uint32_t count8 = static_cast<uint32_t>(std::floor(countM + 0.5));
            count8 = count8 == 0 ? 1 : count8;
            count8 = count8 > (RANS_M - 1) ? (RANS_M - 1) : count8;
            counts[i] = static_cast<uint8_t>(count8);
            total8 += count8;
            // calculate excess fraction of count over integer value
            double fraction = countM - std::floor(countM);
            fractions.push_back({i, fraction});
        }
        // check if we need to correct the total count due to rounding
        int32_t difference8 = static_cast<int32_t>(RANS_M) - static_cast<int32_t>(total8);
        if (difference8 > 0)
        {
            // sort symbol count fractions in descending order
            std::sort(fractions.begin(), fractions.end(), [](const auto &a, const auto &b)
                      { return a.second > b.second; });
            // add difference8 to highest-fraction symbols first
            uint32_t fractionIndex = 0;
            while (difference8 > 0)
            {
                auto symbol = fractions[fractionIndex].first;
                if (counts[symbol] < (RANS_M - 1))
                {
                    counts[symbol]++;
                    difference8--;
                }
                fractionIndex = (fractionIndex + 1) % fractions.size();
            }
        }
        else if (difference8 < 0)
        {
            // sort fractions in ascending order
            std::sort(fractions.begin(), fractions.end(), [](const auto &a, const auto &b)
                      { return a.second < b.second; });
            // subtract difference8 to highest-fraction symbols first
            uint32_t fractionIndex = 0;
            while (difference8 < 0)
            {
                auto symbol = fractions[fractionIndex].first;
                if (counts[symbol] > 1)
                {
                    counts[symbol]--;
                    difference8++;
                }
                fractionIndex = (fractionIndex + 1) % fractions.size();
            }
        }
        const uint32_t finalTotal8 = std::accumulate(counts.begin(), counts.end(), uint32_t(0));
        REQUIRE(finalTotal8 == RANS_M, std::runtime_error, "Counts failed to sum to RANS_M");
        return counts;
    }

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
        for (auto c : src)
        {
            histogram[c]++;
        }
        // check if we have only a single symbol
        uint32_t nrOfSymbols = 0;
        uint8_t singleSymbol = 0;
        for (std::size_t i = 0; i < RANS_ALPHABET_SIZE; ++i)
        {
            if (histogram[i] > 0)
            {
                singleSymbol = i;
                nrOfSymbols++;
            }
        }
        // if we have only a single symbol, change to a different encoding
        if (nrOfSymbols == 1)
        {
            dst.push_back(RANS_HEADER_MODE_SINGLE);
            dst.push_back(singleSymbol);
            return dst;
        }
        // else calculate counts
        auto counts = calculateCounts(histogram);
        // store marker for 256-count header mode
        dst.push_back(RANS_HEADER_MODE_256);
        // store frequency table in header
        std::copy(counts.cbegin(), counts.cend(), std::back_inserter(dst));
        // calculate symbol starts
        std::vector<uint32_t> starts(RANS_ALPHABET_SIZE, 0);
        uint32_t currentStart = 0;
        for (uint32_t i = 0; i < RANS_ALPHABET_SIZE; i++)
        {
            starts[i] = currentStart;
            currentStart += counts[i];
        }
        REQUIRE(currentStart == RANS_M, std::runtime_error, "Counts must sum up to RANS_M");
        // reserve worst case size temporary vector (input size + small margin)
        std::vector<uint8_t> temp;
        temp.reserve(src.size() + 16);
        // encode backwards
        uint32_t x = RANS_L;
        for (auto it = src.crbegin(); it != src.crend(); ++it)
        {
            uint8_t sym = *it;
            uint32_t c = counts[sym];
            uint32_t start = starts[sym];
            REQUIRE(c > 0, std::runtime_error, "Zero-count symbol in encoder");
            // Renormalize: emit bytes while x >= c * RANS_L
            while (x >= (c << RANS_L_BITS))
            {
                temp.push_back(static_cast<uint8_t>(x & 0xFF));
                x >>= 8;
            }
            // Encode update
            uint32_t q = x / c;
            uint32_t r = x % c;
            x = (q << RANS_M_BITS) + r + start;
        }
        // check how many bytes are needed for final state
        const auto x_final = x;
        uint32_t x_statebytes = 0;
        while (x > 0)
        {
            x_statebytes++;
            x >>= 8;
        }
        REQUIRE(x_statebytes <= 4, std::runtime_error, "State too large for 32-bit");
        // flush final state with length prefix directly to destination
        dst.push_back(static_cast<uint8_t>(x_statebytes));
        for (int i = int(x_statebytes) - 1; i >= 0; --i)
        {
            dst.push_back(static_cast<uint8_t>(x_final >> (i * 8)));
        }
        // copy compressed data to end of destination in reverse
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
        REQUIRE(src.size() >= (4 + 1 + 1), std::runtime_error, "Data too small");
        const uint32_t header = *reinterpret_cast<const uint32_t *>(src.data());
        REQUIRE((header & 0xFF) == 0x40, std::runtime_error, "Compression type not rANS (40h)");
        const uint32_t uncompressedSize = (header >> 8);
        REQUIRE(uncompressedSize > 0, std::runtime_error, "Bad uncompressed size");
        std::vector<uint8_t> dst;
        dst.reserve(uncompressedSize);
        // check header byte
        auto srcIt = std::next(src.cbegin(), 4);
        const auto mode = *srcIt++;
        if ((mode & RANS_HEADER_MODE_MASK) == RANS_HEADER_MODE_SINGLE)
        {
            // single symbol mode, just repeat a byte as many times as uncompressed size
            const auto singleSymbol = *srcIt;
            dst.resize(uncompressedSize, singleSymbol);
            return dst;
        }
        // make sure header mode is 256-count mode
        REQUIRE((mode & RANS_HEADER_MODE_MASK) == RANS_HEADER_MODE_256, std::runtime_error, "Bad header mode");
        // read count table
        std::vector<uint8_t> counts(RANS_ALPHABET_SIZE, 0);
        std::copy(srcIt, std::next(srcIt, 256), counts.begin());
        // skip header and frequency table in source data
        srcIt = std::next(srcIt, 256);
        // build starts table
        std::vector<uint32_t> starts(RANS_ALPHABET_SIZE, 0);
        uint32_t currentStart = 0;
        for (size_t i = 0; i < RANS_ALPHABET_SIZE; ++i)
        {
            starts[i] = currentStart;
            currentStart += counts[i];
        }
        REQUIRE(currentStart == RANS_M, std::runtime_error, "Counts must sum up to RANS_M");
        // build symbol table
        std::vector<uint8_t> symbols(RANS_M);
        uint32_t index = 0;
        for (uint32_t s = 0; s < RANS_ALPHABET_SIZE; ++s)
        {
            uint32_t c = counts[s];
            for (uint32_t j = 0; j < c; ++j)
            {
                REQUIRE(index < RANS_M, std::runtime_error, "Symbol index overflow");
                symbols[index++] = static_cast<uint8_t>(s);
            }
        }
        REQUIRE(index == RANS_M, std::runtime_error, "Max. symbol index must be == RANS_M");
        // read rANS state size byte
        const uint8_t stateSize = *srcIt++;
        REQUIRE(stateSize >= 0 && stateSize <= 4, std::runtime_error, "Bad state length");
        REQUIRE(srcIt != src.cend(), std::runtime_error, "Missing rANS state data");
        // read initial rANS state
        uint32_t x = 0;
        for (uint8_t i = 0; i < stateSize; ++i)
        {
            x = (x << 8) | *srcIt++;
        }
        REQUIRE(x >= RANS_M, std::runtime_error, "Initial rANS state too small");
        // decompress data
        while (dst.size() < uncompressedSize)
        {
            uint32_t x_tilde = x & (RANS_M - 1);
            uint8_t symbol = symbols[x_tilde];
            dst.push_back(symbol);
            uint32_t count = counts[symbol];
            REQUIRE(count > 0, std::runtime_error, "Zero-count symbol in decoder");
            uint32_t start = starts[symbol];
            x = count * (x >> RANS_M_BITS) + (x_tilde - start);
            while (x < RANS_L)
            {
                REQUIRE(srcIt != src.cend(), std::runtime_error, "Unexpected end of compressed stream while renormalizing");
                x = (x << 8) | *srcIt++;
            }
        }
        REQUIRE(dst.size() == uncompressedSize, std::runtime_error, "Bad data size after decoding");
        return dst;
    }
}