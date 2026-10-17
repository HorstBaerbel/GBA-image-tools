#include "testmacros.h"

#include "color/optimizedistance.h"
#include "color/rgbf.h"

#include <algorithm>
#include <map>
#include <vector>

using namespace ColorHelpers;

TEST_SUITE("Optimizedistance")

TEST_CASE("insertIndexOptimalChoosesBestPosition")
{
    // Build a squared distance map for 4 colors.
    // Colors 0,1,2 form a tight chain (adjacent distance 1).
    // Color 3 is close to 0 (distance 1) but far from 1 and 2 (distance 9).
    std::map<uint8_t, std::vector<float>> d;
    //        0    1    2    3
    d[0] = {0.0F, 1.0F, 4.0F, 1.0F};
    d[1] = {1.0F, 0.0F, 1.0F, 9.0F};
    d[2] = {4.0F, 1.0F, 0.0F, 9.0F};
    d[3] = {1.0F, 9.0F, 9.0F, 0.0F};
    auto result = insertIndexOptimal({0, 1, 2}, d, 3);
    // The optimal position is at the front: [3,0,1,2] (total adjacent distance 3)
    CATCH_REQUIRE(result == std::vector<uint8_t>({3, 0, 1, 2}));
}

TEST_CASE("insertIndexOptimalReturnsAllElements")
{
    std::map<uint8_t, std::vector<float>> d;
    d[0] = {0.0F, 1.0F, 1.0F};
    d[1] = {1.0F, 0.0F, 1.0F};
    d[2] = {1.0F, 1.0F, 0.0F};
    auto result = insertIndexOptimal({0, 1}, d, 2);
    CATCH_REQUIRE(result.size() == 3);
    // result must contain each index exactly once
    auto sorted = result;
    std::sort(sorted.begin(), sorted.end());
    CATCH_REQUIRE(sorted == std::vector<uint8_t>({0, 1, 2}));
}

TEST_CASE("optimizeColorDistanceReturnsPermutation")
{
    std::vector<Color::RGBf> colors = {
        Color::RGBf(0.0F, 0.0F, 0.0F),
        Color::RGBf(1.0F, 0.0F, 0.0F),
        Color::RGBf(0.0F, 1.0F, 0.0F),
        Color::RGBf(0.0F, 0.0F, 1.0F)};
    auto order = optimizeColorDistance(colors);
    CATCH_REQUIRE(order.size() == colors.size());
    auto sorted = order;
    std::sort(sorted.begin(), sorted.end());
    CATCH_REQUIRE(sorted == std::vector<uint8_t>({0, 1, 2, 3}));
}
