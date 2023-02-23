#pragma once

#include <cstdint>
#include <map>
#include <vector>

template <typename COLOR_TYPE>
class ColorFit
{
    static constexpr std::size_t TABLE_SIZE = 256;

    struct ColorEntry
    {
        COLOR_TYPE color;
        uint64_t importance = 0;
    };
    struct Cluster
    {
        std::vector<ColorEntry> colors;
        uint64_t importance = 0;
    };

public:
    ColorFit(const std::vector<COLOR_TYPE> &colorSpace, COLOR_TYPE invalidColor)
        : m_colorSpace(colorSpace), m_invalidColor(invalidColor)
    {
    }

    // Reduce colors in colorHistogram to nrOfColors while taking into account colorSpace set in constructor
    // @note This is kind of brute-force, can by quite slow and take quite abit of RAM.
    auto reduceColors(const std::map<COLOR_TYPE, uint64_t> &colorHistogram, std::size_t nrOfColors) -> std::vector<COLOR_TYPE>
    {
        // create as many preliminary clusters as colors in colorSpace
        std::map<COLOR_TYPE, Cluster> clusters;
        std::transform(m_colorSpace.cbegin(), m_colorSpace.cend(), std::inserter(clusters, clusters.end()), [](auto center)
                       { return std::make_pair(center, Cluster{{}, 0}); });
        // create as many preliminary distance tables as we have colors in histogram
        std::map<COLOR_TYPE, std::array<COLOR_TYPE, TABLE_SIZE>> distanceColorToCluster;
        std::transform(colorHistogram.cbegin(), colorHistogram.cend(), std::inserter(distanceColorToCluster, distanceColorToCluster.end()), [](const auto &histogramEntry)
                       { return std::make_pair(histogramEntry.first, std::array<COLOR_TYPE, TABLE_SIZE>()); });
        // sort histogram colors into closest clusters in parallel
#pragma omp parallel for
        for (int i = 0; i < static_cast<int>(colorHistogram.size()); i++)
        {
            // pick color from histogram
            auto [color, importance] = *std::next(colorHistogram.cbegin(), i);
            // calculate non-descending distance from color to all clusters
            auto [clusterColor, distanceCache] = getColorToClusterDistances(color, clusters);
            // store distance table (do not store first / closest one)
            distanceColorToCluster[color] = std::move(distanceCache);
#pragma omp critical
            {
                // find cluster closest to color
                auto &closestCluster = clusters[clusterColor];
                // insert color into closest cluster and increase importance of cluster
                closestCluster.colors.emplace_back(ColorEntry{color, importance});
                closestCluster.importance += importance;
            }
        }
        // remove empty clusters
        for (auto clusterIt = clusters.begin(); clusterIt != clusters.end();)
        {
            if (clusterIt->second.importance == 0)
            {
                clusterIt = clusters.erase(clusterIt);
            }
            else
            {
                ++clusterIt;
            }
        }
        // remove least important clusters until we have only nrOfColors clusters left
        while (clusters.size() > nrOfColors)
        {
            // find least-important cluster
            auto leastImportantIt = std::min_element(clusters.cbegin(), clusters.cend(), [](const auto &a, const auto &b)
                                                     { return a.second.importance < b.second.importance; });
            // get cluster colors temporary and remove least important cluster
            const auto leastImportantColors = leastImportantIt->second.colors;
            clusters.erase(leastImportantIt);
            // redistribute colors of least important cluster to closest clusters in parallel
#pragma omp parallel for
            for (int i = 0; i < static_cast<int>(leastImportantColors.size()); i++)
            {
                // get color from cluster
                auto [color, importance] = *std::next(leastImportantColors.cbegin(), i);
                // find cluster closest to color in distance table (non-descending distance)
                auto clusterColor = getClosestCluster(distanceColorToCluster[color], clusters);
                // if we have not found a cluster, we need to regenerate the distance table
                if (clusterColor == m_invalidColor)
                {
                    auto result = getColorToClusterDistances(color, clusters);
                    clusterColor = result.first;
                    // store distance table (do not store first / closest one)
                    // we should be able to do this in parallel, because the entry already exists and we just replace it
                    // also colors are unique and will onyl be accessed by a single thread
                    distanceColorToCluster[color] = std::move(result.second);
                }
#pragma omp critical
                {
                    // find cluster closest to color
                    auto &closestCluster = clusters[clusterColor];
                    // insert color into cluster and increase importance of cluster
                    closestCluster.colors.emplace_back(ColorEntry{color, importance});
                    closestCluster.importance += importance;
                }
            }
        }
        REQUIRE(clusters.size() > 0 && clusters.size() <= nrOfColors, std::runtime_error, "Bad number of clusters");
        // get colors from cluster centers
        std::vector<COLOR_TYPE> result;
        std::transform(clusters.cbegin(), clusters.cend(), std::back_inserter(result), [](const auto &cluster)
                       { return cluster.first; });
        return result;
    }

private:
    auto getColorToClusterDistances(COLOR_TYPE color, const std::map<COLOR_TYPE, Cluster> &clusters) -> std::pair<COLOR_TYPE, std::array<COLOR_TYPE, TABLE_SIZE>>
    {
        // calculate non-descending distance from color to all clusters
        std::map<float, COLOR_TYPE> distances;
        std::transform(clusters.cbegin(), clusters.cend(), std::inserter(distances, distances.end()), [color](const auto &cluster)
                       { return std::make_pair(COLOR_TYPE::distance(color, cluster.first), cluster.first); });
        // get distance cache (do not store first / closest one)
        std::array<COLOR_TYPE, TABLE_SIZE> distanceCache;
        auto distancesIt = std::next(distances.cbegin(), 1);
        for (std::size_t i = 0; i < TABLE_SIZE; i++, ++distancesIt)
        {
            distanceCache[i] = distancesIt != distances.cend() ? distancesIt->second : m_invalidColor;
        }
        return {distances.cbegin()->second, distanceCache};
    }

    auto getClosestCluster(std::array<COLOR_TYPE, TABLE_SIZE> &distancesToCluster, const std::map<COLOR_TYPE, Cluster> &clusters) -> COLOR_TYPE
    {
        // find cluster closest to color in distance table (non-descending distance)
        for (auto &color : distancesToCluster)
        {
            // check if this is a valid entry
            if (color != m_invalidColor)
            {
                // check if cluster exists
                if (clusters.find(color) == clusters.cend())
                {
                    // cluster marked as valid, but not found. mark as invalid in distance table
                    color = m_invalidColor;
                }
                {
                    // marked as valid and found, return it
                    return color;
                }
            }
        }
        return m_invalidColor;
    }

    const COLOR_TYPE m_invalidColor;
    std::vector<COLOR_TYPE> m_colorSpace;
};
