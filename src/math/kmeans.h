#pragma once

#include "exception.h"

#include <limits>
#include <random>
#include <vector>

namespace Kmeans
{
    /// @brief Initialize clusters using maxmimin algorithm
    /// See: Amber Abernathy, M. Emre Celebi 2022, The incremental online k-means clustering algorithm and its application to color quantization
    /// https://uca.edu/cse/files/2022/06/The_Incremental_Online_K_Means_Clustering_Algorithm_and_Its_Application_to_Color_Quantization.pdf
    /// https://github.com/AmberAbernathy/Color_Quantization
    template <class CLUSTER_TYPE, class POSITION_TYPE>
    auto initMaximin(const std::vector<POSITION_TYPE> &positions, const std::size_t nrOfClusters) -> std::vector<CLUSTER_TYPE>
    {
        REQUIRE(nrOfClusters > 0, std::runtime_error, "Number of clusters must be > 0");
        std::vector<CLUSTER_TYPE> clusters;
        // calculate bounding box of data
        BoundingBox<POSITION_TYPE> positionBounds(positions.front());
        std::for_each(positions.cbegin(), positions.cend(), [&positionBounds](auto p)
                      { positionBounds |= p; });
        // start with cluster center in the middle
        clusters.push_back(CLUSTER_TYPE());
        clusters.back().center = 0.5F * (positionBounds.min() + positionBounds.max());
        // calculate additional cluster centers using Maximin initialization method
        std::vector<float> objectClosestCenterDistance(positions.size(), std::numeric_limits<float>::max()); // this is the distance to the closest cluster center yet encountered for this position
        for (std::size_t ci = 1; ci < nrOfClusters; ++ci)
        {
            const auto prevClusterCenter = clusters.back().center;
            auto maxDistancePosition = positions.front();
            auto maxDistanceToCenter = std::numeric_limits<float>::lowest();
            for (int pi = 0; pi < static_cast<int>(positions.size()); pi++)
            {
                const auto &position = positions.at(pi);
                const auto distanceToPrevCenter = POSITION_TYPE::mse(position, prevClusterCenter);
                if (distanceToPrevCenter < objectClosestCenterDistance.at(pi))
                {
                    objectClosestCenterDistance.at(pi) = distanceToPrevCenter;
                }
                if (maxDistanceToCenter < objectClosestCenterDistance.at(pi))
                {
                    maxDistanceToCenter = objectClosestCenterDistance.at(pi);
                    maxDistancePosition = position;
                }
            }
            clusters.push_back(CLUSTER_TYPE());
            clusters.back().center = maxDistancePosition;
        }
        REQUIRE(clusters.size() == nrOfClusters, std::runtime_error, "Failed build expected number of clusters");
    }

    /// @brief Run online k-mean algorithm on clusters
    /// See: Amber Abernathy, M. Emre Celebi 2022, The incremental online k-means clustering algorithm and its application to color quantization
    /// https://uca.edu/cse/files/2022/06/The_Incremental_Online_K_Means_Clustering_Algorithm_and_Its_Application_to_Color_Quantization.pdf
    /// https://github.com/AmberAbernathy/Color_Quantization
    template <class CLUSTER_TYPE, class POSITION_TYPE>
    auto onlineKmeans(std::vector<CLUSTER_TYPE> &clusters, const std::vector<POSITION_TYPE> &positions, const float learnRateExponent) -> void
    {
        // clear all cluster weights
        for (auto &cluster : clusters)
        {
            cluster.weight = 0;
        }
        // generate a random value that is coprime with positions.size()
        // See: https://lemire.me/blog/2017/09/18/visiting-all-values-in-an-array-exactly-once-in-random-order/
        std::size_t lcpA = 0;
        do
        {
            lcpA = 1 + std::rand() % (positions.size() - 1);
        } while (std::gcd(lcpA, positions.size()) != 1);
        // generate a random value from 0 to positions.size() - 1
        const std::size_t lcpB = std::rand() % positions.size();
        // add positions to clusters
        for (int i = 0; i < static_cast<int>(positions.size()); i++)
        {
            // get pseudo-random position
            const auto position = positions.at((static_cast<std::size_t>(i) * lcpA + lcpB) % positions.size());
            // find closest cluster center
            auto bestClusterIndex = std::numeric_limits<int>::max();
            auto bestClusterDistance = std::numeric_limits<float>::max();
            for (int clusterIndex = 0; clusterIndex < static_cast<int>(clusters.size()); clusterIndex++)
            {
                const auto distanceToCluster = POSITION_TYPE::mse(position, clusters.at(clusterIndex).center);
                if (distanceToCluster < bestClusterDistance)
                {
                    bestClusterDistance = distanceToCluster;
                    bestClusterIndex = clusterIndex;
                }
            }
            // update cluster center
            auto &cluster = clusters.at(bestClusterIndex);
            cluster.weight++;
            const float learnRate = std::powf(cluster.weight, -learnRateExponent);
            cluster.center += learnRate * (position - cluster.center);
        }
    }
}
