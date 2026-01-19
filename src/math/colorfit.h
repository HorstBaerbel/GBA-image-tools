#pragma once

#include "boundingbox.h"
#include "color/colorhelpers.h"
#include "color/gamma.h"
#include "exception.h"
#include "math/histogram.h"
#include "math/kmeans.h"
#include "statistics/csvio.h"

#include <Eigen/Core>

#include <algorithm>
#include <cstdint>
#include <iostream>
#include <map>
#include <set>
#include <vector>

// #define DUMP_STATS

template <typename PIXEL_TYPE>
class ColorFit
{
    static constexpr float LearnRateExponent = 0.5;
    using COLOR_TYPE = Color::CIELabf;

    // Cluster containing color objects
    struct Cluster
    {
        COLOR_TYPE center;               // Cluster center / linear color
        uint32_t weight = 0;             // Weight of all colors in cluster
        std::vector<PIXEL_TYPE> objects; // sRGB colors closest to cluster
    };

public:
    /// @brief Construct color fit object
    /// @param colorSpace All colors of targrt color space as sRGB colors
    ColorFit(const std::vector<PIXEL_TYPE> &colorSpace)
        : m_colorSpace(colorSpace), m_colorSpaceLinear(Color::convertTo<COLOR_TYPE>(Color::srgbToLinear(colorSpace)))
    {
    }

    /// @brief Reduce colors in colorHistogram to nrOfColors while taking into account colorSpace set in constructor.
    /// How it works:
    /// - Initialize cluster centers using Maximin initialization method for Batch- / Online-k-means
    /// - Run Online-k-means
    /// - Snap colors to input color space
    /// - Run Online-k-means again to improve result
    /// See: Amber Abernathy, M. Emre Celebi 2022, The incremental online k-means clustering algorithm and its application to color quantization
    /// https://uca.edu/cse/files/2022/06/The_Incremental_Online_K_Means_Clustering_Algorithm_and_Its_Application_to_Color_Quantization.pdf
    /// https://github.com/AmberAbernathy/Color_Quantization
    /// @note This can be quite slow and take a bit of RAM. You have been warned...
    /// @param pixels sRGB input pixels
    /// @param nrOfColors Number of colors to reduce input colors to
    /// @returns Mapping of reduced color -> input colors. This might not contain exactly nrOfColors, but possibly less due to restricted color space
    auto reduceColors(const std::vector<PIXEL_TYPE> &pixels, const std::size_t nrOfColors) const -> std::map<PIXEL_TYPE, std::vector<PIXEL_TYPE>>
    {
        REQUIRE(nrOfColors > 1 && nrOfColors <= 256, std::runtime_error, "Bad number of colors. Must be in range [2,256]");
        // std::cout << "Building histogram..." << std::endl;
        const std::map<PIXEL_TYPE, uint64_t> colorHistogram = Histogram::buildHistogram(pixels);
        // check if we already have enough colors
        if (colorHistogram.size() <= nrOfColors)
        {
            std::cout << "Data already has less than requested " << nrOfColors << " (" << colorHistogram.size() << ")" << std::endl;
            std::map<PIXEL_TYPE, std::vector<PIXEL_TYPE>> colorMapping;
            std::transform(colorHistogram.cbegin(), colorHistogram.cend(), std::inserter(colorMapping, colorMapping.end()), [](const auto &entry)
                           { return std::make_pair(entry.first, std::vector<PIXEL_TYPE>{entry.first}); });
            return colorMapping;
        }
        // std::cout << "Reducing " << colorHistogram.size() << " colors to " << nrOfColors << "..." << std::endl;
        // Linearize pixel colors
        const auto linearPixels = Color::convertTo<COLOR_TYPE>(Color::srgbToLinear(pixels));
        // get simpler array than our map and convert to linear color space
        std::vector<std::pair<PIXEL_TYPE, COLOR_TYPE>> linearColors;
        linearColors.reserve(colorHistogram.size());
        for (const auto &color : colorHistogram)
        {
            linearColors.push_back({color.first, Color::convertTo<COLOR_TYPE>(Color::srgbToLinear(color.first))});
        }
        // ---------- Maximin initialization ----------
        // calculate bounding box of data
        BoundingBox<COLOR_TYPE> colorBounds(linearColors.front().second);
        std::for_each(linearColors.cbegin(), linearColors.cend(), [&colorBounds](auto c)
                      { colorBounds |= c.second; });
        // start with cluster center in the middle
        std::vector<Cluster> clusters;
        clusters.push_back(Cluster{0.5F * (colorBounds.min() + colorBounds.max()), 0, {}});
        // calculate additional cluster centers using Maximin initialization method
        std::vector<float> objectClosestCenterDistance(linearColors.size(), std::numeric_limits<float>::max()); // this is the distance to the closest cluster center yet encountered for this object
        for (std::size_t ci = 1; ci < nrOfColors; ++ci)
        {
            const auto prevClusterCenter = clusters.back().center;
            auto maxDistanceColor = linearColors.front().second;
            auto maxDistanceToCenter = std::numeric_limits<float>::lowest();
            // #pragma omp parallel for
            for (int oi = 0; oi < static_cast<int>(linearColors.size()); oi++)
            {
                const auto &color = linearColors.at(oi);
                const auto distanceToPrevCenter = COLOR_TYPE::mse(color.second, prevClusterCenter);
                if (distanceToPrevCenter < objectClosestCenterDistance.at(oi))
                {
                    objectClosestCenterDistance.at(oi) = distanceToPrevCenter;
                }
                // #pragma omp critical
                if (maxDistanceToCenter < objectClosestCenterDistance.at(oi))
                {
                    maxDistanceToCenter = objectClosestCenterDistance.at(oi);
                    maxDistanceColor = color.second;
                }
            }
            clusters.push_back(Cluster{maxDistanceColor, 0, {}});
        }
        REQUIRE(clusters.size() == nrOfColors, std::runtime_error, "Failed build expected number of clusters");
        // run Online-k-means
        Kmeans::onlineKmeans(clusters, linearPixels, LearnRateExponent);
        // snap all cluster centers to color space
#pragma omp parallel for schedule(dynamic)
        for (int ci = 0; ci < static_cast<int>(clusters.size()); ci++)
        {
            auto &cluster = clusters.at(ci);
            if (!m_colorSpaceLinear.empty())
            {
                cluster.center = ColorHelpers::getClosestColor(cluster.center, m_colorSpaceLinear);
            }
        }
        // run Online-k-means again to improve result
        Kmeans::onlineKmeans(clusters, linearPixels, LearnRateExponent);
        // add colors to closest cluster
#pragma omp parallel for
        for (int ci = 0; ci < static_cast<int>(linearColors.size()); ci++)
        {
            const auto &color = linearColors.at(ci);
            // find closest cluster center
            auto bestClusterIndex = std::numeric_limits<int>::max();
            auto bestClusterDistance = std::numeric_limits<float>::max();
            for (int clusterIndex = 0; clusterIndex < static_cast<int>(clusters.size()); clusterIndex++)
            {
                const auto distanceToCluster = COLOR_TYPE::mse(color.second, clusters.at(clusterIndex).center);
                if (distanceToCluster < bestClusterDistance)
                {
                    bestClusterDistance = distanceToCluster;
                    bestClusterIndex = clusterIndex;
                }
            }
            // add object to cluster
            auto &cluster = clusters.at(bestClusterIndex);
#pragma omp critical
            {
                cluster.objects.push_back(color.first);
            }
        }
#ifdef DUMP_STATS
        dumpToCSV(clusters, colorHistogram);
#endif
        // return mapping from reduced set of colors to original colors
        std::map<PIXEL_TYPE, std::vector<PIXEL_TYPE>> colorMapping;
        for (const auto &cluster : clusters)
        {
            // find color in linearized color space map
            auto closestColorIt = m_colorSpaceLinear.cend();
            double closestDistance = std::numeric_limits<float>::max();
            auto cIt = std::next(m_colorSpaceLinear.cbegin());
            while (cIt != m_colorSpaceLinear.cend())
            {
                auto colorDistance = COLOR_TYPE::mse(*cIt, cluster.center);
                if (closestDistance > colorDistance)
                {
                    closestDistance = colorDistance;
                    closestColorIt = cIt;
                }
                ++cIt;
            }
            // get index in linear color space map
            const auto colorSpaceIndex = std::distance(m_colorSpaceLinear.cbegin(), closestColorIt);
            // use index to get original sRGB color space color
            const auto colorSpaceColor = m_colorSpace.at(colorSpaceIndex);
            // check which mapping to add colors to
            auto cmIt = colorMapping.find(colorSpaceColor);
            if (cmIt != colorMapping.end())
            {
                // color mapping already exists. append colors
                std::copy(cluster.objects.cbegin(), cluster.objects.cend(), std::back_inserter(cmIt->second));
            }
            else
            {
                // new mapping. just copy
                colorMapping[colorSpaceColor] = cluster.objects;
            }
        }
        // now here the number of mappings can be less then the nrOfColors (clusters getting merged), but we need to have all colors mapped
        const auto nrOfMappedColors = std::accumulate(colorMapping.cbegin(), colorMapping.cend(), uint64_t(0), [](const auto &a, const auto &b)
                                                      { return a + b.second.size(); });
        REQUIRE(nrOfMappedColors == linearColors.size(), std::runtime_error, "Failed to map all input colors (" << nrOfMappedColors << " of " << linearColors.size() << ")");
        return colorMapping;
    }

private:
    static auto dumpToCSV(const std::vector<Cluster> &clusters, const std::map<PIXEL_TYPE, uint64_t> &colorHistogram) -> void
    {
        std::ofstream csvObjects("colorfit_objects.csv");
        IO::CSV::writeCSV(csvObjects, {"r", "g", "b", "csscolor", "clusterindex", "clustercolor"}, colorHistogram, [&clusters](decltype(*colorHistogram.cbegin()) o, std::size_t index)
                          { switch (index) {
                            case 0:
                                return std::to_string(o.first.R());
                            case 1:
                                return std::to_string(o.first.G());
                            case 2:
                                return std::to_string(o.first.B());
                            case 3:
                                return "#" + o.first.toHex();
                            case 4:
                            {
                                auto cIt = clusters.cbegin();
                                for (; cIt != clusters.cend(); ++cIt)
                                {
                                    if (std::find_if(cIt->objects.cbegin(), cIt->objects.cend(), [value = o.first](auto object)
                                                     { return object == value; }) != cIt->objects.cend())
                                    {
                                        break;
                                    }
                                }
                                return std::to_string(cIt == clusters.cend() ? -10L : static_cast<int64_t>(std::distance(clusters.cbegin(), cIt)));
                            }
                            case 5:
                            {
                                auto cIt = clusters.cbegin();
                                for (; cIt != clusters.cend(); ++cIt)
                                {
                                    if (std::find_if(cIt->objects.cbegin(), cIt->objects.cend(), [value = o.first](auto object)
                                                     { return object == value; }) != cIt->objects.cend())
                                    {
                                        break;
                                    }
                                }
                                return Color::convertTo<Color::XRGB8888>(Color::linearToSrgb(std::next(clusters.cbegin(), cIt)->center)).toHex();
                            }
                            default:
                                THROW(std::runtime_error, "Bad index");
                            } });
        std::ofstream csvClusters("colorfit_clusters.csv");
        IO::CSV::writeCSV(csvClusters, {"r", "g", "b", "csscolor"}, clusters, [](decltype(clusters.front()) c, std::size_t index)
                          { switch (index) {
                            case 0:
                                return std::to_string(Color::linearToSrgb(c.center).R());
                            case 1:
                                return std::to_string(Color::linearToSrgb(c.center).G());
                            case 2:
                                return std::to_string(Color::linearToSrgb(c.center).B());
                            case 3:
                                return "#" + Color::convertTo<Color::XRGB8888>(Color::linearToSrgb(c.center)).toHex();
                            default:
                                THROW(std::runtime_error, "Bad index");
                            } });
    }

    const std::vector<PIXEL_TYPE> m_colorSpace;       // The sRGB color space passed in constructor
    const std::vector<COLOR_TYPE> m_colorSpaceLinear; // The color space color linearized to linearized sRGB
};
