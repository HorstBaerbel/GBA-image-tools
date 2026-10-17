#include "testmacros.h"

#include "math/histogram.h"

#include <cmath>
#include <cstdint>
#include <map>
#include <vector>

TEST_SUITE("Histogram")

TEST_CASE("buildHistogramCounts")
{
    std::vector<uint8_t> data = {0, 1, 1, 2, 2, 2, 5};
    auto hist = Histogram::buildHistogram(data);
    CATCH_REQUIRE(hist.size() == 4);
    CATCH_REQUIRE(hist.at(0) == 1);
    CATCH_REQUIRE(hist.at(1) == 2);
    CATCH_REQUIRE(hist.at(2) == 3);
    CATCH_REQUIRE(hist.at(5) == 1);
    // Values not present in data must not appear in the histogram.
    CATCH_REQUIRE(hist.find(3) == hist.end());
    CATCH_REQUIRE(hist.find(4) == hist.end());
}

TEST_CASE("buildHistogramEmpty")
{
    std::vector<uint8_t> data;
    auto hist = Histogram::buildHistogram(data);
    CATCH_REQUIRE(hist.empty());
}

TEST_CASE("buildHistogramKeepEmptyFillsAllValues")
{
    std::vector<uint8_t> data = {0, 255, 255};
    auto hist = Histogram::buildHistogramKeepEmpty(data);
    // uint8_t spans 0..255, so all 256 buckets must exist.
    CATCH_REQUIRE(hist.size() == 256);
    CATCH_REQUIRE(hist.at(0) == 1);
    CATCH_REQUIRE(hist.at(255) == 2);
    // Unseen values must be present with a count of zero.
    CATCH_REQUIRE(hist.at(1) == 0);
    CATCH_REQUIRE(hist.at(128) == 0);
}

TEST_CASE("normalizeHistogramSumsToOne")
{
    std::vector<uint8_t> data = {1, 1, 2, 3};
    auto hist = Histogram::buildHistogram(data);
    auto norm = Histogram::normalizeHistogram(hist);
    CATCH_REQUIRE(norm.size() == hist.size());
    float sum = 0.0F;
    for (const auto &entry : norm)
        sum += entry.second;
    CATCH_REQUIRE(std::abs(sum - 1.0F) < 1e-6F);
    // 1 appears twice out of 4 total.
    CATCH_REQUIRE(std::abs(norm.at(1) - 0.5F) < 1e-6F);
    CATCH_REQUIRE(std::abs(norm.at(2) - 0.25F) < 1e-6F);
    CATCH_REQUIRE(std::abs(norm.at(3) - 0.25F) < 1e-6F);
}

TEST_CASE("buildHistogramCustomFloatType")
{
    std::vector<int> data = {7, 7, 7, 7, 9};
    auto hist = Histogram::buildHistogram<int, uint32_t>(data);
    CATCH_REQUIRE(hist.at(7) == 4);
    CATCH_REQUIRE(hist.at(9) == 1);
    // Default float type is uint64_t.
    auto histDefault = Histogram::buildHistogram(data);
    CATCH_REQUIRE(histDefault.at(7) == 4);
}
