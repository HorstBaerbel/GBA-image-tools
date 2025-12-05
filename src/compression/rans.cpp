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
    // 2: RLE with literal blocks
    //    Use the most significant bit (MSB) of the control byte as a flag:
    //    Flag 0 (Literal block): 0 + 6-bit count. The following count bytes are literal data (raw copies)
    //    Flag 1 (Run block): 1 + 3-bit count, 4-bit value. The value is repeated count + 1 times. if value == 15 an extra value byte follows

    template <typename CountType>
    static std::vector<CountType> calculateCounts(const std::vector<uint32_t> &histogram)
    {
        auto total = std::accumulate(histogram.cbegin(), histogram.cend(), uint32_t(0));
        REQUIRE(total >= 1, std::runtime_error, "Empty input for scaling");
        // build counts and fractions table
        std::vector<std::pair<uint8_t, double>> fractions; // stores (symbol, fraction above integer count)
        fractions.reserve(RANS_ALPHABET_SIZE);
        uint32_t totalM = 0;
        std::vector<CountType> counts(RANS_ALPHABET_SIZE, 0);
        for (size_t i = 0; i < RANS_ALPHABET_SIZE; ++i)
        {
            // ignore synbols that do not occur in the data
            if (histogram[i] == 0)
            {
                continue;
            }
            // calculate floating-point count in [0, RANS_M]
            double countF = (histogram[i] * double(RANS_M)) / static_cast<double>(total);
            // calculate 8-bit count and clamp to [0, RANS_M - 1]
            uint32_t countM = static_cast<uint32_t>(std::floor(countF + 0.5));
            countM = countM == 0 ? 1 : countM;
            countM = countM > (RANS_M - 1) ? (RANS_M - 1) : countM;
            counts[i] = countM;
            totalM += countM;
            // calculate excess fraction of count over integer value
            double fraction = countF - std::floor(countF);
            fractions.push_back({i, fraction});
        }
        // check if we need to correct the total count due to rounding
        REQUIRE(totalM == std::accumulate(counts.cbegin(), counts.cend(), uint32_t(0)), std::runtime_error, "Counts difference");
        int32_t differenceM = static_cast<int32_t>(RANS_M) - static_cast<int32_t>(totalM);
        if (differenceM > 0)
        {
            // sort symbol count fractions in descending order
            std::sort(fractions.begin(), fractions.end(), [](const auto &a, const auto &b)
                      { return a.second > b.second; });
            // add differenceM to highest-fraction symbols first
            uint32_t fractionIndex = 0;
            while (differenceM > 0)
            {
                auto symbol = fractions[fractionIndex].first;
                if (counts[symbol] < (RANS_M - 1))
                {
                    counts[symbol]++;
                    differenceM--;
                }
                fractionIndex = (fractionIndex + 1) % fractions.size();
            }
        }
        else if (differenceM < 0)
        {
            // sort fractions in ascending order
            std::sort(fractions.begin(), fractions.end(), [](const auto &a, const auto &b)
                      { return a.second < b.second; });
            // subtract differenceM to highest-fraction symbols first
            uint32_t fractionIndex = 0;
            while (differenceM < 0)
            {
                auto symbol = fractions[fractionIndex].first;
                if (counts[symbol] > 1)
                {
                    counts[symbol]--;
                    differenceM++;
                }
                fractionIndex = (fractionIndex + 1) % fractions.size();
            }
        }
        const uint32_t finalTotalM = std::accumulate(counts.cbegin(), counts.cend(), uint32_t(0));
        REQUIRE(finalTotalM == RANS_M, std::runtime_error, "Counts failed to sum to RANS_M");
        return counts;
    }

    auto encodeRANS(const std::vector<uint8_t> &src) -> std::vector<uint8_t>
    {
        REQUIRE(!src.empty(), std::runtime_error, "Data too small");
        REQUIRE(src.size() < (1 << 24), std::runtime_error, "Data too big");
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
#if RANS_M_BITS <= 8
        auto counts = calculateCounts<uint8_t>(histogram);
#else
        auto counts = calculateCounts<uint16_t>(histogram);
#endif
        // store marker for 256-count header mode
        dst.push_back(RANS_HEADER_MODE_256);
        // store frequency table in header
        const auto countsSize8 = counts.size() * sizeof(decltype(counts.front()));
        dst.resize(dst.size() + countsSize8);
        std::memcpy(dst.data() + dst.size() - countsSize8, reinterpret_cast<const uint8_t *>(counts.data()), countsSize8);
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
        for (auto srcIt = src.crbegin(); srcIt != src.crend(); ++srcIt)
        {
            uint8_t symbol = *srcIt;
            uint32_t count = counts[symbol];
            uint32_t start = starts[symbol];
            REQUIRE(count > 0, std::runtime_error, "Zero-count symbol in encoder");
            // renormalize: emit bytes while x >= x_max
            const uint32_t x_max = ((RANS_L >> RANS_M_BITS) << 8) * count;
            while (x >= x_max)
            {
                temp.push_back(static_cast<uint8_t>(x));
                x >>= 8;
            }
            // Encode update
            uint32_t q = x / count;
            uint32_t r = x % count;
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
#if RANS_M_BITS <= 8
        std::vector<uint8_t> counts(RANS_ALPHABET_SIZE, 0);
#else
        std::vector<uint16_t> counts(RANS_ALPHABET_SIZE, 0);
#endif
        const auto countsSize8 = counts.size() * sizeof(decltype(counts.front()));
        std::memcpy(counts.data(), &*srcIt, countsSize8);
        // skip header and frequency table in source data
        srcIt = std::next(srcIt, countsSize8);
        // build starts table
        std::vector<uint16_t> starts(RANS_ALPHABET_SIZE, 0);
        uint16_t currentStart = 0;
        for (size_t i = 0; i < RANS_ALPHABET_SIZE; ++i)
        {
            starts[i] = currentStart;
            currentStart += counts[i];
        }
        REQUIRE(currentStart == RANS_M, std::runtime_error, "Counts must sum up to RANS_M");
        // build symbol table
        std::vector<uint8_t> symbols(RANS_M);
        uint16_t index = 0;
        for (uint32_t symbol = 0; symbol < RANS_ALPHABET_SIZE; ++symbol)
        {
            auto count = counts[symbol];
            for (uint32_t j = 0; j < count; ++j)
            {
                REQUIRE(index < RANS_M, std::runtime_error, "Symbol index overflow");
                symbols[index++] = static_cast<uint8_t>(symbol);
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
            auto count = counts[symbol];
            REQUIRE(count > 0, std::runtime_error, "Zero-count symbol in decoder");
            auto start = starts[symbol];
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