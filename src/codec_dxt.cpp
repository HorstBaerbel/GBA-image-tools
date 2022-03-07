#include "codec_dxt.h"

#include "colorhelpers.h"
#include "exception.h"

#include <Eigen/Core>
#include <Eigen/Dense>
#include <array>
#include <deque>

#include <iostream>

std::vector<std::vector<uint8_t>> DXT::RGB555DistanceSqrCache;

using Colord = Eigen::Vector3d;

// Found here: https://gist.github.com/ialhashim/0a2554076a6cf32831ca
// See also: https://zalo.github.io/blog/line-fitting/
template <class Vector3>
std::pair<Vector3, Vector3> bestLineFromColors(const std::vector<Vector3> &colors)
{
    // copy coordinates to matrix in Eigen format
    const size_t NrOfAtoms = colors.size();
    Eigen::Matrix<typename Vector3::Scalar, Eigen::Dynamic, Eigen::Dynamic> centers(NrOfAtoms, 3);
    for (size_t i = 0; i < NrOfAtoms; ++i)
    {
        centers.row(i) = colors[i];
    }
    // center on mean
    Vector3 origin = centers.colwise().mean();
    Eigen::MatrixXd centered = centers.rowwise() - origin.transpose();
    // calculate adjoint
    Eigen::MatrixXd cov = centered.adjoint() * centered;
    Eigen::SelfAdjointEigenSolver<Eigen::MatrixXd> eig(cov);
    Vector3 axis = eig.eigenvectors().col(2).normalized();
    return {origin, axis};
}

Colord truncToGrid(const Colord &v)
{
    Colord r;
    // clamp to [0,31]
    r.x() = std::trunc(v.x() < 0.0 ? 0.0 : (v.x() > 31.0 ? 31.0 : v.x()));
    r.y() = std::trunc(v.y() < 0.0 ? 0.0 : (v.y() > 31.0 ? 31.0 : v.y()));
    r.z() = std::trunc(v.z() < 0.0 ? 0.0 : (v.z() > 31.0 ? 31.0 : v.z()));
    return r;
}

Colord roundToGrid(const Colord &v)
{
    Colord r;
    // clamp to [0,31]
    r.x() = v.x() < 0.0 ? 0.0 : (v.x() > 31.0 ? 31.0 : v.x());
    r.y() = v.y() < 0.0 ? 0.0 : (v.y() > 31.0 ? 31.0 : v.y());
    r.z() = v.z() < 0.0 ? 0.0 : (v.z() > 31.0 ? 31.0 : v.z());
    // round to grid point
    r.x() = std::trunc(r.x() + 0.5);
    r.y() = std::trunc(r.y() + 0.5);
    r.z() = std::trunc(r.z() + 0.5);
    return r;
}

Colord toVector(uint16_t color)
{
    return {static_cast<double>((color & 0x7C00) >> 10), static_cast<double>((color & 0x3E0) >> 5), static_cast<double>(color & 0x1F)};
}

uint16_t toPixel(const Colord &color)
{
    uint16_t r = static_cast<uint16_t>(color.x());
    uint16_t g = static_cast<uint16_t>(color.y());
    uint16_t b = static_cast<uint16_t>(color.z());
    return (r << 10) | (g << 5) | b;
}

double colorDistance(const Colord &a, const Colord &b)
{
    if (a == b)
    {
        return 0.0;
    }
    auto ra = a.x() / 31.0;
    auto rb = b.x() / 31.0;
    auto r = 0.5 * (ra + rb);
    auto dR = ra - rb;
    auto dG = (a.y() / 31.0) - (b.y() / 31.0);
    auto dB = (a.z() / 31.0) - (b.z() / 31.0);
    return (2.0 + r) * dR * dR + 4.0 * dG * dG + (3.0 - r) * dB * dB;
} // max:  (2   + 1) *  1 *  1 + 4   *  1 *  1 + (3   - 1) *  1 *  1 = 3 + 4 + 2 = 9

// This is basically the "range fit" method from here: http://www.sjbrown.co.uk/2006/01/19/dxt-compression-techniques/
std::vector<uint8_t> DXT::encodeBlockDXTG2(const uint16_t *start, uint32_t pixelsPerScanline, const std::vector<std::vector<uint8_t>> &distanceSqrMap)
{
    REQUIRE(pixelsPerScanline % 4 == 0, std::runtime_error, "Image width must be a multiple of 4 for DXT compression");
    // get block colors for all 16 pixels
    std::vector<Colord> colors(16);
    auto cIt = colors.begin();
    auto pixel = start;
    for (int y = 0; y < 4; y++)
    {
        *cIt++ = toVector(pixel[0]);
        *cIt++ = toVector(pixel[1]);
        *cIt++ = toVector(pixel[2]);
        *cIt++ = toVector(pixel[3]);
        pixel += pixelsPerScanline;
    }
    // calculate line fit through RGB color space
    auto originAndAxis = bestLineFromColors(colors);
    // project all points onto the line
    std::vector<Colord> colorsOnLine(16);
    std::transform(colors.cbegin(), colors.cend(), colorsOnLine.begin(), [origin = originAndAxis.first, axis = originAndAxis.second](const auto &color)
                   { return (color - origin).dot(axis) / axis.dot(axis) * axis; });
    // calculate signed distance from origin
    std::vector<double> distanceFromOrigin(16);
    std::transform(colorsOnLine.cbegin(), colorsOnLine.cend(), distanceFromOrigin.begin(), [origin = originAndAxis.first, axis = originAndAxis.second](const auto &color)
                   { return axis.dot(color); });
    // get the distance of endpoints c0 and c1 on line
    auto minMaxDistance = std::minmax_element(distanceFromOrigin.cbegin(), distanceFromOrigin.cend());
    auto indexC0 = std::distance(distanceFromOrigin.cbegin(), minMaxDistance.first);
    auto indexC1 = std::distance(distanceFromOrigin.cbegin(), minMaxDistance.second);
    // get colors c0 and c1 on line and round to grid
    auto c0 = roundToGrid(originAndAxis.first + colorsOnLine[indexC0]);
    auto c1 = roundToGrid(originAndAxis.first + colorsOnLine[indexC1]);
    Colord endpoints[4] = {c0, c1, {}, {}};
    /*if (toPixel(endpoints[0]) > toPixel(endpoints[1]))
    {
        std::swap(endpoints[0], endpoints[1]);
    }*/
    // calculate intermediate colors c2 and c3
    endpoints[2] = roundToGrid(Colord((c0.cwiseProduct(Colord(2, 2, 2)) + c1).cwiseQuotient(Colord(3, 3, 3))));
    endpoints[3] = roundToGrid(Colord((c0 + c1.cwiseProduct(Colord(2, 2, 2))).cwiseQuotient(Colord(3, 3, 3))));
    // calculate minimum distance for all colors to endpoints
    std::array<uint32_t, 16> bestIndices = {0};
    for (uint32_t ci = 0; ci < 16; ++ci)
    {
        // calculate minimum distance for each index for this color
        double bestColorDistance = std::numeric_limits<double>::max();
        for (uint32_t ei = 0; ei < 4; ++ei)
        {
            auto indexDistance = colorDistance(colors[ci], endpoints[ei]);
            // check if result improved
            if (bestColorDistance > indexDistance)
            {
                bestColorDistance = indexDistance;
                bestIndices[ci] = ei;
            }
        }
    }
    // build result data
    std::vector<uint8_t> result(2 * 2 + 16 * 2 / 8);
    // add color endpoints c0 and c1
    auto data16 = reinterpret_cast<uint16_t *>(result.data());
    *data16++ = toBGR555(toPixel(endpoints[0]));
    *data16++ = toBGR555(toPixel(endpoints[1]));
    // add index data in reverse
    uint32_t indices = 0;
    for (auto iIt = bestIndices.crbegin(); iIt != bestIndices.crend(); ++iIt)
    {
        indices = (indices << 2) | *iIt;
    }
    *(reinterpret_cast<uint32_t *>(data16)) = indices;
    return result;
}

using Cluster = std::pair<Colord, std::vector<Colord>>;

/*double DistanceSqr(const std::array<Cluster, 4> &clusters)
{
    auto dist = 0.0;
    for (auto clusterIt = clusters.begin(); clusterIt != clusters.end(); ++clusterIt)
    {
        dist += std::accumulate(clusterIt->second.cbegin(), clusterIt->second.cend(), 0.0, [center = clusterIt->first](const auto &v, const auto &color)
                                { return v + DistanceSqr(center, color); });
    }
    return dist;
}

// This is basically the "cluster fit" method from here: http://www.sjbrown.co.uk/2006/01/19/dxt-compression-techniques/
std::vector<uint8_t> DXT::encodeBlockDXTG3(const uint16_t *start, uint32_t pixelsPerScanline, const std::vector<std::vector<uint8_t>> &distanceSqrMap)
{
    REQUIRE(pixelsPerScanline % 4 == 0, std::runtime_error, "Image width must be a multiple of 4 for DXT compression");
    // get block colors for all 16 pixels
    std::array<uint16_t, 16> colors16 = {0};
    auto c16It = colors16.begin();
    std::vector<Colord> colors(16);
    auto cIt = colors.begin();
    auto pixel = start;
    for (int y = 0; y < 4; y++)
    {
        *c16It++ = pixel[0];
        *cIt++ = toVector<double>(pixel[0]);
        *c16It++ = pixel[1];
        *cIt++ = toVector<double>(pixel[1]);
        *c16It++ = pixel[2];
        *cIt++ = toVector<double>(pixel[2]);
        *c16It++ = pixel[3];
        *cIt++ = toVector<double>(pixel[3]);
        pixel += pixelsPerScanline;
    }
    // calculate line fit through RGB color space
    auto originAndAxis = bestLineFromColors(colors);
    // project all points onto the line
    std::array<Colord, 14> colorsOnLine;
    std::transform(colors.cbegin(), colors.cend(), colorsOnLine.begin(), [origin = originAndAxis.first, axis = originAndAxis.second](const auto &color)
                   { return origin + (color - origin).dot(axis) / axis.dot(axis) * axis; });
    // calculate signed distance from origin
    std::array<double, 16> distanceFromOrigin;
    std::transform(colorsOnLine.cbegin(), colorsOnLine.cend(), distanceFromOrigin.begin(), [origin = originAndAxis.first, axis = originAndAxis.second](const auto &color)
                   { return color.norm() * axis.dot(color); });
    // get the min/max point indices
    auto minMaxDistance = std::minmax_element(distanceFromOrigin.cbegin(), distanceFromOrigin.cend());
    auto p0 = colorsOnLine[std::distance(distanceFromOrigin.cbegin(), minMaxDistance.first)];
    auto p1 = colorsOnLine[std::distance(distanceFromOrigin.cbegin(), minMaxDistance.second)];
    // we split the line into 4 parts and use the centers as cluster centers
    std::array<Cluster, 4> clusters;
    // put the initial cluster centers at 1/8, 3/8, 5/8 and 7/8 along the principle axis
    clusters[0].first = (p1 - p0) * 1.0 / 8.0 + p0;
    clusters[1].first = (p1 - p0) * 3.0 / 8.0 + p0;
    clusters[2].first = (p1 - p0) * 5.0 / 8.0 + p0;
    clusters[3].first = (p1 - p0) * 7.0 / 8.0 + p0;
    // put colors into clusters
    for (const auto &color : colors)
    {
        // find cluster with minimal color-distance
        auto minDist = std::numeric_limits<double>::max();
        auto minIt = clusters.begin();
        for (auto clusterIt = clusters.begin(); clusterIt != clusters.end(); ++clusterIt)
        {
            auto dist = DistanceSqr(clusterIt->first, color);
            if (minDist > dist)
            {
                minDist = dist;
                minIt = clusterIt;
            }
        }
        // insert point into cluster
        minIt->second.push_back(color);
        // recalculate cluster center
        minIt->first = std::accumulate(minIt->second.cbegin(), minIt->second.cend(), Eigen::Vector3d::Zero());
        minIt->first /= minIt->second.size();
    }
    // calculate current distance
    auto bestDist = DistanceSqr(clusters);
    auto bestClusters = clusters;
    // try to optimize clusters
    int32_t iterationsLeft = 8;
    while (iterationsLeft-- > 0)
    {
        // find
    }
// calculate intermediate colors c2 and c3
uint16_t endpoints[4] = {colors16[indexC0], colors16[indexC1], 0, 0};
endpoints[2] = lerpRGB555(endpoints[0], endpoints[1], 1.0 / 3.0);
endpoints[3] = lerpRGB555(endpoints[0], endpoints[1], 2.0 / 3.0);
// calculate minimum distance for all colors to endpoints
std::array<uint32_t, 16> bestIndices = {0};
for (uint32_t ci = 0; ci < 16; ++ci)
{
    const auto &distMapColorI = distanceSqrMap[colors16[ci]];
    // calculate minimum distance for each index for this color
    uint8_t bestColorDistance = std::numeric_limits<uint8_t>::max();
    for (uint32_t ei = 0; ei < 4; ++ei)
    {
        auto indexDistance = distMapColorI[endpoints[ei]];
        // check if result improved
        if (bestColorDistance > indexDistance)
        {
            bestColorDistance = indexDistance;
            bestIndices[ci] = ei;
        }
    }
}
// build result data
std::vector<uint8_t> result(2 * 2 + 16 * 2 / 8);
// add color endpoints c0 and c1
auto data16 = reinterpret_cast<uint16_t *>(result.data());
*data16++ = toBGR555(endpoints[0]);
*data16++ = toBGR555(endpoints[1]);
// add index data in reverse
uint32_t indices = 0;
for (auto iIt = bestIndices.crbegin(); iIt != bestIndices.crend(); ++iIt)
{
    indices = (indices << 2) | *iIt;
}
*(reinterpret_cast<uint32_t *>(data16)) = indices;
return result;
}*/

auto DXT::encodeDXTG(const std::vector<uint16_t> &image, uint32_t width, uint32_t height) -> std::vector<uint8_t>
{
    REQUIRE(width % 4 == 0, std::runtime_error, "Image width must be a multiple of 4 for DXT compression");
    REQUIRE(height % 4 == 0, std::runtime_error, "Image height must be a multiple of 4 for DXT compression");
    // check if squared distance map has been allocated
    if (RGB555DistanceSqrCache.empty())
    {
        RGB555DistanceSqrCache = std::move(RGB555DistanceSqrTable());
    }
    // build y-position table for parallel for
    std::vector<uint32_t> ys(height / 4);
    std::generate(ys.begin(), ys.end(), [y = 0]() mutable
                  {
                          y += 4;
                          return y - 4; });
    // compress to DXT1. we get 8 bytes per 4x4 block / 16 pixels
    const auto yStride = width * 8 / 16;
    const auto nrOfBlocks = width / 4 * height / 4;
    std::vector<uint8_t> dxtData(nrOfBlocks * 8);
#pragma omp parallel for
    for (int y = 0; y < height; y += 4)
    {
        for (uint32_t x = 0; x < width; x += 4)
        {
            auto block = encodeBlockDXTG2(image.data() + y * width + x, width, RGB555DistanceSqrCache);
            std::copy(block.cbegin(), block.cend(), std::next(dxtData.begin(), y * yStride + x * 8 / 4));
        }
    }
    // split data into colors and indices for better compression
    std::vector<uint8_t> data(nrOfBlocks * 8);
    // std::vector<uint16_t> colorData(nrOfBlocks * 2);
    auto colorPtr16 = reinterpret_cast<uint16_t *>(data.data());
    // std::vector<uint16_t> indexData(nrOfBlocks * 2);
    auto indexPtr16 = reinterpret_cast<uint16_t *>(data.data() + nrOfBlocks * 4);
    auto srcPtr16 = reinterpret_cast<const uint16_t *>(dxtData.data());
    for (uint32_t i = 0; i < nrOfBlocks; i++)
    {
        *colorPtr16++ = *srcPtr16++;
        *colorPtr16++ = *srcPtr16++;
        *indexPtr16++ = *srcPtr16++;
        *indexPtr16++ = *srcPtr16++;
    }
    /*auto srcPtr16 = reinterpret_cast<const uint16_t *>(resultData.data());
    std::vector<uint16_t> uniqueColors;
    for (uint32_t i = 0; i < nrOfBlocks * 2; i++)
    {
        auto color = *srcPtr16++;
        if (std::find(uniqueColors.cbegin(), uniqueColors.cend(), color) == uniqueColors.cend())
        {
            uniqueColors.push_back(color);
        }
    }
    auto srcPtr32 = reinterpret_cast<const uint32_t *>(srcPtr16);
    std::vector<uint32_t> uniqueIndices;
    for (uint32_t i = 0; i < nrOfBlocks; i++)
    {
        auto indices = *srcPtr32++;
        if (std::find(uniqueIndices.cbegin(), uniqueIndices.cend(), indices) == uniqueIndices.cend())
        {
            uniqueIndices.push_back(indices);
        }
    }
    std::cout << "Unique colors: " << uniqueColors.size() << ", indices: " << uniqueIndices.size() << std::endl;*/
#ifdef SLIDING_WINDOW
    // encode colors with moving window
    std::deque<uint16_t> colorWindow;
    srcPtr16 = reinterpret_cast<const uint16_t *>(colorData.data());
    uint32_t colorReuseCount = 0;
    for (uint32_t i = 0; i < nrOfBlocks * 2; i++)
    {
        auto color = *srcPtr16++;
        auto cwIt = std::find(colorWindow.cbegin(), colorWindow.cend(), color);
        if (cwIt != colorWindow.cend())
        {
            auto dist = std::distance(colorWindow.cbegin(), cwIt);
            data.push_back(static_cast<uint8_t>(dist));
            colorWindow.erase(cwIt);
            colorWindow.push_front(color);
            colorReuseCount++;
        }
        else
        {
            data.push_back(static_cast<uint8_t>(color >> 8));
            data.push_back(static_cast<uint8_t>(color & 0xFF));
            if (colorWindow.size() >= 256)
            {
                colorWindow.pop_back();
            }
            colorWindow.push_front(color);
        }
    }
    // std::cout << "Reused " << colorReuseCount << " of " << nrOfBlocks * 2 << " colors (" << ((colorReuseCount * 100) / (nrOfBlocks * 2)) << "%)" << std::endl;
#endif
#ifdef SLIDING_WINDOW
    std::deque<uint16_t> indexWindow;
    srcPtr16 = reinterpret_cast<const uint16_t *>(indexData.data());
    uint32_t indexReuseCount = 0;
    for (uint32_t i = 0; i < nrOfBlocks * 2; i++)
    {
        auto indices = *srcPtr16++;
        auto iwIt = std::find(indexWindow.cbegin(), indexWindow.cend(), indices);
        if (iwIt != indexWindow.cend())
        {
            auto dist = std::distance(indexWindow.cbegin(), iwIt);
            data.push_back(static_cast<uint8_t>(dist));
            indexWindow.erase(iwIt);
            indexWindow.push_front(indices);
            indexReuseCount++;
        }
        else
        {
            data.push_back(static_cast<uint8_t>(indices >> 8));
            data.push_back(static_cast<uint8_t>(indices & 0xFF));
            if (indexWindow.size() >= 256)
            {
                indexWindow.pop_back();
            }
            indexWindow.push_front(indices);
        }
    }
    // std::cout << "Reused " << indexReuseCount << " of " << nrOfBlocks * 2 << " indices (" << ((indexReuseCount * 100) / (nrOfBlocks * 2)) << "%)" << std::endl;
#endif
    return data;
}

auto decodeDXTG(const std::vector<uint8_t> &data, uint32_t width, uint32_t height) -> std::vector<uint8_t>
{
    return {};
}