#include "testmacros.h"

#include "math/boundingbox.h"
#include "math/correlate.h"
#include "math/dct.h"
#include "math/kdtree.h"
#include "math/kmeans.h"
#include "math/linefit.h"

#include <Eigen/Core>
#include <Eigen/Dense>

#include <array>
#include <cmath>
#include <limits>
#include <vector>

#include <catch2/matchers/catch_matchers_floating_point.hpp>

TEST_SUITE("Math")

using Vec3f = Eigen::Vector3f;

namespace
{
    // Minimal position type: a 3D vector that provides the static mse() used by the kmeans/boundingbox templates
    class Position : public Vec3f
    {
    public:
        Position() : Vec3f(0.0F, 0.0F, 0.0F) {}
        Position(float x, float y, float z) : Vec3f(x, y, z) {}
        template <class... Types>
        Position(const Eigen::CwiseBinaryOp<Types...> &op) : Vec3f(op.matrix()) {}
        static auto mse(const Position &a, const Position &b) -> float
        {
            return (a - b).squaredNorm();
        }
    };

    struct Cluster
    {
        Position center{};
        float weight = 0;
    };
}

// ---------------------------------------------------------------------------
// kdtree
// ---------------------------------------------------------------------------

using Point2 = point<float, 2>;
using Point3 = point<float, 3>;

TEST_CASE("kdtree point distance")
{
    Point2 a{0.0F, 0.0F};
    Point2 b{3.0F, 4.0F};
    // distance returns squared distance
    CATCH_REQUIRE_THAT(a.distance(b), Catch::Matchers::WithinAbs(25.0, 0.0001F));
    CATCH_REQUIRE_THAT(b.distance(a), Catch::Matchers::WithinAbs(25.0, 0.0001F));
    CATCH_REQUIRE_THAT(a.distance(a), Catch::Matchers::WithinAbs(0.0, 0.0001F));
}

TEST_CASE("kdtree point get")
{
    Point3 p{1.0F, 2.0F, 3.0F};
    CATCH_REQUIRE_THAT(p.get(0), Catch::Matchers::WithinAbs(1.0F, 0.0001F));
    CATCH_REQUIRE_THAT(p.get(1), Catch::Matchers::WithinAbs(2.0F, 0.0001F));
    CATCH_REQUIRE_THAT(p.get(2), Catch::Matchers::WithinAbs(3.0F, 0.0001F));
}

TEST_CASE("kdtree nearest from iterators")
{
    std::vector<Point3> points = {
        Point3{0.0F, 0.0F, 0.0F},
        Point3{10.0F, 10.0F, 10.0F},
        Point3{1.0F, 1.0F, 1.0F},
        Point3{9.0F, 9.0F, 9.0F},
        Point3{5.0F, 5.0F, 5.0F}};
    kdtree<float, 3> tree(points.cbegin(), points.cend());
    CATCH_REQUIRE_FALSE(tree.empty());
    CATCH_REQUIRE(tree.visited() == 0);
    auto &nearest = tree.nearest(Point3{4.9F, 4.9F, 4.9F});
    CATCH_REQUIRE_THAT(nearest.get(0), Catch::Matchers::WithinAbs(5.0F, 0.0001F));
    CATCH_REQUIRE_THAT(tree.distance(), Catch::Matchers::WithinAbs(std::sqrt(0.03), 1e-4)); // sqrt(3*0.01)
    CATCH_REQUIRE(tree.visited() > 0);
}

TEST_CASE("kdtree nearest from generator")
{
    std::size_t n = 100;
    std::size_t i = 0;
    kdtree<float, 3> tree([&i]()
                          { auto p = Point3{static_cast<float>(i), static_cast<float>(i), 0.0F}; ++i; return p; },
                          n);
    CATCH_REQUIRE_FALSE(tree.empty());
    // query a point that is not in the tree, nearest should be the max point
    auto &nearest = tree.nearest(Point3{200.0F, 200.0F, 0.0F});
    CATCH_REQUIRE_THAT(nearest.get(0), Catch::Matchers::WithinAbs(99.0F, 0.0001F));
}

TEST_CASE("kdtree empty throws")
{
    std::vector<Point3> empty;
    kdtree<float, 3> tree(empty.cbegin(), empty.cend());
    CATCH_REQUIRE(tree.empty());
    CATCH_REQUIRE_THROWS_AS(tree.nearest(Point3{0, 0, 0}), std::logic_error);
}

// ---------------------------------------------------------------------------
// DCT
// ---------------------------------------------------------------------------

TEST_CASE("dct constant block has only DC term")
{
    // A constant 4x4 block: only the (0,0) coefficient should be non-zero
    std::array<float, 16> block{};
    for (auto &v : block)
        v = 2.0F;
    auto out = DCT::dct<4, 4, float, float>(block, [](float v)
                                            { return v; });
    // DC term of DCT-II for constant c over NxM: N*M*c
    CATCH_REQUIRE_THAT(out[0], Catch::Matchers::WithinAbs(16.0F * 2.0F, 0.0001F));
    for (std::size_t i = 1; i < out.size(); ++i)
        CATCH_REQUIRE_THAT(out[i], Catch::Matchers::WithinAbs(0.0F, 1e-4));
}

TEST_CASE("dct zero block is zero")
{
    std::array<float, 16> block{};
    auto out = DCT::dct<4, 4, float, float>(block, [](float v)
                                            { return v; });
    for (auto v : out)
        CATCH_REQUIRE_THAT(v, Catch::Matchers::WithinAbs(0.0F, 1e-5));
}

// ---------------------------------------------------------------------------
// Correlation
// ---------------------------------------------------------------------------

TEST_CASE("crosscorrelate identical vectors is 1")
{
    std::array<Eigen::Vector3d, 8> a{};
    for (std::size_t i = 0; i < a.size(); ++i)
        a[i] = Eigen::Vector3d(static_cast<double>(i), static_cast<double>(i) + 1, static_cast<double>(i) - 1);
    auto r = Correlation::crosscorrelate0(a, a);
    for (int i = 0; i < 3; ++i)
        CATCH_REQUIRE_THAT(r[i], Catch::Matchers::WithinAbs(1.0, 1e-6));
}

TEST_CASE("crosscorrelate negated vectors is -1")
{
    std::array<Eigen::Vector3d, 8> a{};
    std::array<Eigen::Vector3d, 8> b{};
    for (std::size_t i = 0; i < a.size(); ++i)
    {
        a[i] = Eigen::Vector3d(static_cast<double>(i), static_cast<double>(i) + 1, static_cast<double>(i) - 1);
        b[i] = -a[i];
    }
    auto r = Correlation::crosscorrelate0(a, b);
    for (int i = 0; i < 3; ++i)
        CATCH_REQUIRE_THAT(r[i], Catch::Matchers::WithinAbs(-1.0, 1e-6));
}

TEST_CASE("convolve0 zero-shift convolution")
{
    std::array<Eigen::Vector3d, 4> a{};
    std::array<Eigen::Vector3d, 4> b{};
    for (std::size_t i = 0; i < a.size(); ++i)
    {
        a[i] = Eigen::Vector3d(static_cast<double>(i + 1), 0.0, 0.0); // (1,0,0),(2,0,0),(3,0,0),(4,0,0)
        b[i] = Eigen::Vector3d(static_cast<double>(i + 1), 0.0, 0.0); // (1,0,0),(2,0,0),(3,0,0),(4,0,0)
    }
    auto r = Correlation::convolve0(a, b);
    // True convolution: a[0]·b[3] + a[1]·b[2] + a[2]·b[1] + a[3]·b[0]
    //                  = 1*4 + 2*3 + 3*2 + 4*1 = 4+6+6+4 = 20
    CATCH_REQUIRE_THAT(r[0], Catch::Matchers::WithinAbs(20.0, 0.0001));
    CATCH_REQUIRE_THAT(r[1], Catch::Matchers::WithinAbs(0.0, 0.0001));
    CATCH_REQUIRE_THAT(r[2], Catch::Matchers::WithinAbs(0.0, 0.0001));
}

// ---------------------------------------------------------------------------
// BoundingBox
// ---------------------------------------------------------------------------

TEST_CASE("boundingbox default")
{
    BoundingBox<Position> bb; // per default at 0,0,0
    bb |= Position(0, 0, 0);
    CATCH_REQUIRE(bb.min() == Position(0, 0, 0));
    CATCH_REQUIRE(bb.max() == Position(0, 0, 0));
    CATCH_REQUIRE_THAT(bb.size(), Catch::Matchers::WithinAbs(0.0F, 0.0001F));
}

TEST_CASE("boundingbox default and single point")
{
    BoundingBox<Position> bb(Position(1, 2, 3));
    bb |= Position(1, 2, 3);
    CATCH_REQUIRE(bb.min() == Position(1, 2, 3));
    CATCH_REQUIRE(bb.max() == Position(1, 2, 3));
    CATCH_REQUIRE_THAT(bb.size(), Catch::Matchers::WithinAbs(0.0F, 0.0001F));
}

TEST_CASE("boundingbox expands with points")
{
    BoundingBox<Position> bb;
    bb |= Position(0, 0, 0);
    bb |= Position(10, 5, 2);
    bb |= Position(-3, 8, 1);
    CATCH_REQUIRE(bb.min() == Position(-3, 0, 0));
    CATCH_REQUIRE(bb.max() == Position(10, 8, 2));
    CATCH_REQUIRE(bb.size() > 0.0F);
}

TEST_CASE("boundingbox free operator pipe")
{
    BoundingBox<Position> bb;
    bb |= Position(1, 1, 1);
    auto result = bb | Position(4, 2, 3);
    CATCH_REQUIRE(result.min() == Position(0, 0, 0));
    CATCH_REQUIRE(result.max() == Position(4, 2, 3));
    // original is unchanged (free operator copies)
    CATCH_REQUIRE(bb.min() == Position(0, 0, 0));
    CATCH_REQUIRE(bb.max() == Position(1, 1, 1));
}

// ---------------------------------------------------------------------------
// lineFit
// ---------------------------------------------------------------------------

TEST_CASE("linefit collinear points")
{
    // points along the x axis
    std::vector<Vec3f> points = {
        Vec3f(0, 0, 0), Vec3f(1, 0, 0), Vec3f(2, 0, 0),
        Vec3f(3, 0, 0), Vec3f(4, 0, 0)};
    auto [origin, axis] = lineFit(points);
    CATCH_REQUIRE_THAT(origin.x(), Catch::Matchers::WithinAbs(2.0F, 0.0001F));
    CATCH_REQUIRE_THAT(origin.y(), Catch::Matchers::WithinAbs(0.0F, 0.0001F));
    CATCH_REQUIRE_THAT(origin.z(), Catch::Matchers::WithinAbs(0.0F, 0.0001F));
    // axis should be along x
    CATCH_REQUIRE_THAT(std::abs(axis.x()), Catch::Matchers::WithinAbs(1.0F, 1e-4));
    CATCH_REQUIRE_THAT(std::abs(axis.y()), Catch::Matchers::WithinAbs(0.0F, 1e-4));
    CATCH_REQUIRE_THAT(std::abs(axis.z()), Catch::Matchers::WithinAbs(0.0F, 1e-4));
}

TEST_CASE("linefit array overload")
{
    std::array<Vec3f, 5> points = {
        Vec3f(0, 0, 0), Vec3f(0, 1, 0), Vec3f(0, 2, 0),
        Vec3f(0, 3, 0), Vec3f(0, 4, 0)};
    auto [origin, axis] = lineFit(points);
    CATCH_REQUIRE_THAT(origin.y(), Catch::Matchers::WithinAbs(2.0F, 0.0001F));
    CATCH_REQUIRE_THAT(std::abs(axis.y()), Catch::Matchers::WithinAbs(1.0F, 1e-4));
}

// ---------------------------------------------------------------------------
// kmeans
// ---------------------------------------------------------------------------

TEST_CASE("kmeans initMaximin")
{
    std::vector<Position> positions = {
        Position(0, 0, 0), Position(10, 0, 0), Position(0, 10, 0), Position(10, 10, 0)};
    auto clusters = Kmeans::initMaximin<Cluster, Position>(positions, 3);
    CATCH_REQUIRE(clusters.size() == 3);
    // first cluster center is the midpoint of the bounding box
    CATCH_REQUIRE(clusters.front().center == Position(5, 5, 0));
}

TEST_CASE("kmeans initMaximin rejects zero clusters")
{
    std::vector<Position> positions = {Position(0, 0, 0), Position(1, 1, 1)};
    auto throwFunc = [&positions]()
    { Kmeans::initMaximin<Cluster, Position>(positions, 0); };
    CATCH_REQUIRE_THROWS_AS(throwFunc(), std::runtime_error);
}

TEST_CASE("kmeans onlineKmeans assigns weights")
{
    std::vector<Position> positions = {
        Position(0, 0, 0), Position(0.1F, 0, 0), Position(10, 0, 0), Position(10.1F, 0, 0)};
    std::vector<Cluster> clusters(2);
    clusters[0].center = Position(0, 0, 0);
    clusters[1].center = Position(10, 0, 0);
    Kmeans::onlineKmeans(clusters, positions, 0.5F);
    // every position was assigned exactly once, so total weight == positions.size()
    float totalWeight = clusters[0].weight + clusters[1].weight;
    CATCH_REQUIRE_THAT(totalWeight, Catch::Matchers::WithinAbs(static_cast<float>(positions.size()), 0.0001F));
    // centers should stay close to their respective clusters
    CATCH_REQUIRE(clusters[0].center.x() < 5.0F);
    CATCH_REQUIRE(clusters[1].center.x() > 5.0F);
}
