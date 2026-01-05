#pragma once

#include "boundingbox.h"
#include "exception.h"
#include "statistics/csvio.h"

#include <Eigen/Core>

#include <algorithm>
#include <cstdint>
#include <iostream>
#include <map>
#include <vector>

// #define USE_SNAPPED_COLORS
#define ADD_OBJECTS_BY_WEIGHT

template <typename PIXEL_TYPE>
class ColorFit
{
    static constexpr std::size_t InvalidClusterIndex = std::numeric_limits<std::size_t>::max();

    // Color object
    struct ColorObject
    {
        const double weight = 0.0; // Normalized number of occurrences in histogram
        std::size_t clusterIndex = InvalidClusterIndex;
    };

    // Cluster containing color objects
    struct Cluster
    {
        Color::RGBf center;              // Cluster center / color
        double weight = 0.0;             // Weight of all objects in cluster combined
        std::vector<PIXEL_TYPE> objects; // Objects / colors in cluster
        std::vector<float> errors;       // Error an object has against cluster center
    };

public:
    ColorFit(const std::vector<PIXEL_TYPE> &colorSpace)
        : m_colorSpace(colorSpace)
    {
    }

    /// @brief Reduce colors in colorHistogram to nrOfColors while taking into account colorSpace set in constructor.
    /// How it works:
    /// - initialize cluster centers using Maximin initialization method for Batch- / Online-k-means
    /// - run modified Online-k-means
    /// See: Amber Abernathy, M. Emre Celebi 2022, The incremental online k-means clustering algorithm and its application to color quantization
    /// https://uca.edu/cse/files/2022/06/The_Incremental_Online_K_Means_Clustering_Algorithm_and_Its_Application_to_Color_Quantization.pdf
    /// https://github.com/AmberAbernathy/Color_Quantization
    /// @note This can be quite slow and take quite a bit of RAM. You have been warned...
    /// @param colorHistogram Histogram of all input colors
    /// @param nrOfColors Number of colors to reduce input colors to
    /// @returns Mapping of reduced color -> input colors. This might not contain exactly nrOfColors, but possibly less due to restricted color space
    auto reduceColors(const std::map<PIXEL_TYPE, uint64_t> &colorHistogram, const std::size_t nrOfColors) const -> std::map<PIXEL_TYPE, std::vector<PIXEL_TYPE>>
    {
        REQUIRE(nrOfColors > 1 && nrOfColors <= 256, std::runtime_error, "Bad number of colors. Must be in range [2,256]");
        // get max histogram count
        const auto maxCount = std::max_element(colorHistogram.cbegin(), colorHistogram.cend(), [](const auto &a, const auto &b)
                                               { return a.second < b.second; })
                                  ->second;
        // round the number to the nearst power of ten
        const double maxCountPow10 = std::pow(10.0, std::round(std::log10(maxCount)));
        // create as many objects as colors in histogram. normalize weight and clamp to [0,1]
        std::map<PIXEL_TYPE, ColorObject> objects;
        std::transform(colorHistogram.cbegin(), colorHistogram.cend(), std::inserter(objects, objects.end()), [maxCountPow10](const auto &entry)
                       { return std::make_pair(entry.first, ColorObject{entry.second >= maxCountPow10 ? 1.0 : static_cast<double>(entry.second) / maxCountPow10, InvalidClusterIndex}); });
        // calculate bounding box of data
        BoundingBox<Color::RGBf> colorBounds(Color::convertTo<Color::RGBf>(colorHistogram.cbegin()->first));
        std::for_each(colorHistogram.cbegin(), colorHistogram.cend(), [&colorBounds](const auto &c)
                      { colorBounds |= Color::convertTo<Color::RGBf>(c.first); });
#ifdef ADD_OBJECTS_BY_WEIGHT
        std::vector<PIXEL_TYPE> objectsByCount;
        objectsByCount.reserve(objects.size());
        for (const auto &object : objects)
        {
            objectsByCount.push_back(object.first);
        }
        std::sort(objectsByCount.begin(), objectsByCount.end(), [&colorHistogram](const auto &a, const auto &b)
                  { return colorHistogram.at(a) > colorHistogram.at(b); });
#endif
        // ---------- Maximin initialization ----------
        // start with cluster center in the middle
        std::vector<Cluster> clusters;
        clusters.push_back(Cluster{0.5 * (colorBounds.min() + colorBounds.max()), 0.0, {}});
        // calculate additional cluster centers using Maximin initialization method
        std::vector<float> objectClosestCenterDistance(objects.size(), std::numeric_limits<float>::max()); // this is the distance to the closest cluster center yet encountered for this object
        for (std::size_t ci = 1; ci < nrOfColors; ++ci)
        {
            const auto prevClusterCenter = clusters.back().center;
            auto maxDistanceObject = objects.cbegin()->first;
            auto maxDistanceToCenter = std::numeric_limits<float>::lowest();
#pragma omp parallel for
            for (int oi = 0; oi < static_cast<int>(objects.size()); oi++)
            {
                auto objectIt = std::next(objects.cbegin(), oi);
                const auto objectPosition = Color::convertTo<Color::RGBf>(objectIt->first);
                const auto distanceToPrevCenter = Color::RGBf::mse(objectPosition, prevClusterCenter);
                if (distanceToPrevCenter < objectClosestCenterDistance.at(oi))
                {
                    objectClosestCenterDistance.at(oi) = distanceToPrevCenter;
                }
#pragma omp critical
                if (maxDistanceToCenter < objectClosestCenterDistance.at(oi))
                {
                    maxDistanceToCenter = objectClosestCenterDistance.at(oi);
                    maxDistanceObject = objectIt->first;
                }
            }
#ifdef USE_SNAPPED_COLORS
            // snap center to closest colorspace color
            auto snappedColor = getClosestColor(maxDistanceObject, m_colorSpace);
            clusters.push_back(Cluster{Color::convertTo<Color::RGBf>(snappedColor), 0.0, {}});
#else
            clusters.push_back(Cluster{Color::convertTo<Color::RGBf>(maxDistanceObject), 0.0, {}});
#endif
        }
        // ---------- Run Online-k-means ----------
#ifndef ADD_OBJECTS_BY_WEIGHT
        // generate a random value that is coprime with objects.size()
        // See: https://lemire.me/blog/2017/09/18/visiting-all-values-in-an-array-exactly-once-in-random-order/
        std::size_t lcpA = 0;
        do
        {
            lcpA = 1 + std::rand() % (objects.size() - 1);
        } while (std::gcd(lcpA, objects.size()) != 1);
        // generate a random value from 0 to objects.size() - 1
        const std::size_t lcpB = std::rand() % objects.size();
#endif
        // add objects to clusters
        // This differs from standard Online-k-means as:
        // Were not sampling an image, but a histogram, so our sampling rate is set to 1
        // The learning rate is also 1 and centers shift based on object weight
        for (int i = 0; i < static_cast<int>(objects.size()); i++)
        {
#ifdef ADD_OBJECTS_BY_WEIGHT
            const auto objectColor = objectsByCount.at(i);
            auto &object = objects.at(objectColor);
#else
            // get pseudo-random object
            const auto objectIndex = (static_cast<std::size_t>(i) * lcpA + lcpB) % objects.size();
            const auto objectColor = std::next(objects.begin(), objectIndex)->first;
            auto &object = objects.at(objectColor);
#endif
            const auto objectCenter = Color::convertTo<Color::RGBf>(objectColor);
            const auto objectWeight = object.weight;
            // find closest cluster center
            auto bestDistanceClusterIndex = std::numeric_limits<int>::max();
            auto bestDistanceToCluster = std::numeric_limits<float>::max();
            for (int clusterIndex = 0; clusterIndex < static_cast<int>(clusters.size()); clusterIndex++)
            {
                const auto distanceToCluster = Color::RGBf::mse(objectCenter, clusters.at(clusterIndex).center);
                if (distanceToCluster < bestDistanceToCluster)
                {
                    bestDistanceToCluster = distanceToCluster;
                    bestDistanceClusterIndex = clusterIndex;
                }
            }
            // add object to cluster
            auto &cluster = clusters.at(bestDistanceClusterIndex);
            cluster.objects.push_back(objectColor);
            object.clusterIndex = bestDistanceClusterIndex;
            // update cluster center
            Color::RGBf clusterCenter(cluster.center.R(), cluster.center.G(), cluster.center.B());
            auto combinedWeight = cluster.weight + objectWeight;
            auto t0 = cluster.weight / combinedWeight;
            auto t1 = 1.0F - t0;
            clusterCenter.R() = t0 * clusterCenter.R() + t1 * objectCenter.R();
            clusterCenter.G() = t0 * clusterCenter.G() + t1 * objectCenter.G();
            clusterCenter.B() = t0 * clusterCenter.B() + t1 * objectCenter.B();
#ifdef USE_SNAPPED_COLORS
            // snap center to closest colorspace color
            PIXEL_TYPE centerColor = Color::convertTo<PIXEL_TYPE>(clusterCenter);
            auto snappedColor = getClosestColor(centerColor, m_colorSpace);
            cluster.center = Color::convertTo<Color::RGBf>(snappedColor);
#else
            cluster.center = clusterCenter;
            cluster.weight = combinedWeight;
#endif
        }
        REQUIRE(clusters.size() == nrOfColors, std::runtime_error, "Failed to find expected number of clusters");
        // update cluster error and calculate overall error
        double overallMse = 0.0;
        double overallCount = 0.0;
        for (auto &cluster : clusters)
        {
            const auto clusterCenter = Color::convertTo<PIXEL_TYPE>(cluster.center);
            for (auto object : cluster.objects)
            {
                auto error = PIXEL_TYPE::mse(object, clusterCenter);
                overallMse += static_cast<double>(error) * objects.at(object).weight;
                ++overallCount;
            }
        }
        overallMse /= overallCount; // calculate mean squared error
        const auto overallPsnr = 10.0 * std::log10(1.0 / overallMse);
        std::cout << "Overall PSNR: " << overallPsnr << " dB" << std::endl;
        // return mapping from reduced set of colors to original colors while snapping all cluster centers to color space
        std::map<PIXEL_TYPE, std::vector<PIXEL_TYPE>> colorMapping;
        for (auto &cluster : clusters)
        {
            PIXEL_TYPE centerColor = Color::convertTo<PIXEL_TYPE>(cluster.center);
            const auto snappedCenter = getClosestColor(centerColor, m_colorSpace);
            cluster.center = Color::convertTo<Color::RGBf>(snappedCenter);
            auto cmIt = colorMapping.find(snappedCenter);
            if (cmIt != colorMapping.end())
            {
                // color mapping already exists. append colors
                std::copy(cluster.objects.cbegin(), cluster.objects.cend(), std::back_inserter(cmIt->second));
            }
            else
            {
                // new mapping. just copy
                colorMapping[snappedCenter] = cluster.objects;
            }
        }
        dumpToCSV(clusters, objects);
        //  now here the number of mappings can be less then the nrOfColors (cluster getting merged), but we need to have all colors mapped
        const auto nrOfMappedColors = std::accumulate(colorMapping.cbegin(), colorMapping.cend(), uint64_t(0), [](const auto &a, const auto &b)
                                                      { return a + b.second.size(); });
        REQUIRE(nrOfMappedColors == colorHistogram.size(), std::runtime_error, "Failed to map all input colors (" << nrOfMappedColors << " of " << colorHistogram.size() << ")");
        return colorMapping;
    }

private:
    static auto getClosestColor(PIXEL_TYPE color, const std::vector<PIXEL_TYPE> &colors) -> PIXEL_TYPE
    {
        // Note: We don't use min_element here due to double distance function evaluations
        // Improvement: Use distance query acceleration structure
        auto colorIt = colors.cbegin();
        auto closestColor = *colorIt;
        double closestDistance = std::numeric_limits<double>::max();
        while (colorIt != colors.cend())
        {
            auto colorDistance = PIXEL_TYPE::mse(*colorIt, color);
            if (closestDistance > colorDistance)
            {
                closestDistance = colorDistance;
                closestColor = *colorIt;
            }
            ++colorIt;
        }
        return closestColor;
    }

    static auto getClusterError(const Cluster &cluster) -> std::vector<float>
    {
        std::vector<float> errors;
        errors.reserve(cluster.objects.size());
        const auto clusterCenter = Color::convertTo<PIXEL_TYPE>(cluster.center);
        for (auto object : cluster.objects)
        {
            errors.push_back(PIXEL_TYPE::mse(object, clusterCenter));
        }
        return errors;
    }

    static auto dumpToCSV(const std::vector<Cluster> &clusters, const std::map<PIXEL_TYPE, ColorObject> &objects) -> void
    {
        std::ofstream csvObjects("colorfit_objects.csv");
        IO::CSV::writeCSV(csvObjects, {"r", "g", "b", "csscolor", "weight", "clusterindex"}, objects, [](decltype(*objects.cbegin()) o, std::size_t index)
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
                                return std::to_string(10.0 * o.second.weight);
                            case 5:
                                return std::to_string(o.second.clusterIndex >= InvalidClusterIndex ? -10L : static_cast<int64_t>(o.second.clusterIndex));
                            default:
                                THROW(std::runtime_error, "Bad index");
                            } });
        std::ofstream csvClusters("colorfit_clusters.csv");
        IO::CSV::writeCSV(csvClusters, {"r", "g", "b", "csscolor"}, clusters, [](decltype(clusters.front()) c, std::size_t index)
                          { switch (index) {
                            case 0:
                                return std::to_string(Color::convertTo<PIXEL_TYPE>(c.center).R());
                            case 1:
                                return std::to_string(Color::convertTo<PIXEL_TYPE>(c.center).G());
                            case 2:
                                return std::to_string(Color::convertTo<PIXEL_TYPE>(c.center).B());
                            case 3:
                                return "#" + Color::convertTo<PIXEL_TYPE>(c.center).toHex();
                            default:
                                THROW(std::runtime_error, "Bad index");
                            } });
    }

    std::vector<PIXEL_TYPE> m_colorSpace;
};
