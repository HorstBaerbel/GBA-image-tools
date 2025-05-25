#pragma once

#include "boundingbox.h"
#include "exception.h"
#include "io/csvio.h"

#include <Eigen/Core>

#include <algorithm>
#include <cstdint>
#include <map>
#include <vector>
#include <iostream>

template <typename PIXEL_TYPE>
class ColorFit
{
    static constexpr std::size_t MinNeighbourCount = 5;       // For more than 2D use minPts ~= 2*D, See: https://stackoverflow.com/questions/12893492/choosing-eps-and-minpts-for-dbscan-r
    static constexpr double MinNeigbourDistance = 0.0001;     // Min. epsilon / color distance, See: https://stats.stackexchange.com/questions/88872/a-routine-to-choose-eps-and-minpts-for-dbscan
    static constexpr double MaxNeigbourDistance = 0.0003;     // Max. epsilon / color distance
    static constexpr double HistogramCountPercent = 99.9;     // Histogram percentage for median calculation
    static constexpr double MinImportantOutlierWeight = 0.5;  // Min. normalized weight at which outliers are considered "important"
    static constexpr double ClusterDimensionMin = 0.1 * 0.1;  // Min. dimension of single cluster. Prevents clusters from getting too big
    static constexpr double ClusterDimensionMax = 0.4 * 0.4;  // Max. dimension of single cluster. Prevents clusters from getting too big

    static constexpr std::size_t InvalidClusterIndex = std::numeric_limits<std::size_t>::max();

    // Color object
    struct ColorObject
    {
        enum class Type
        {
            Outlier = 0, // Object not within MaxNeigbourDistance of a CoreObject
            NonCore = 1, // Object reachable from a CoreObject, but has less than MinNeighbourCount neighbours
            Core = 2     // Object has at least MinNeighbourCount neighbours
        };
        Type type = Type::Outlier;
        double weight = 0.0;                // Normalized number of occurrences in histogram
        std::vector<PIXEL_TYPE> neighbours; // Neighbour in less than MaxNeigbourDistance
        bool visited = false;
        std::size_t clusterIndex = InvalidClusterIndex;
    };

    // Cluster containing color objects
    struct Cluster
    {
        PIXEL_TYPE center;
        double weight = 0.0;
        std::vector<PIXEL_TYPE> objects;      // Objects in cluster
        BoundingBox<PIXEL_TYPE> objectBounds; // Range of object coordinates
    };

public:
    ColorFit(const std::vector<PIXEL_TYPE> &colorSpace)
        : m_colorSpace(colorSpace)
    {
    }

    // Reduce colors in colorHistogram to nrOfColors while taking into account colorSpace set in constructor.
    // How it works:
    // - Add all histogram colors as objects storing their normalized weight (how often it occurs) and all neighbours closer than allowedNeighbourDistance
    // - Run DBSCAN( https://de.wikipedia.org/wiki/DBSCAN) to find clusters and important outliers
    // - Calculate weighted cluster center points using object weights and snap center point to colorspace color
    // - Snap important outlier objects to colorspace color
    // - Repeat while adjusting allowedNeighbourDistance until important outliers + clusters ~= nrOfColors
    // @note This can be quite slow and take quite a bit of RAM. You have been warned...
    /// @param colorHistogram Histogram of all input colors
    /// @param nrOfColors Number of colors to reduce input colors to
    /// @returns Mapping of reduced color -> input colors
    auto reduceColors(const std::map<PIXEL_TYPE, uint64_t> &colorHistogram, std::size_t nrOfColors) const -> std::map<PIXEL_TYPE, std::vector<PIXEL_TYPE>>
    {
        REQUIRE(nrOfColors > 1 && nrOfColors <= 256, std::runtime_error, "Bad number of colors. Must be in range [2,256]");
        std::size_t iterationsLeft = 5;
        // Calculate maximum allowed cluster size depending on number of colors
        const double maxClusterDimension = static_cast<double>(nrOfColors) / 256.0 * ClusterDimensionMin + (1.0 - static_cast<double>(nrOfColors) / 256.0) * ClusterDimensionMax;
        // Start with medium allowed object distance
        double minNeighbourDistance = MinNeigbourDistance;
        double maxNeighbourDistance = MaxNeigbourDistance;
        // calculate median range of histogram values for weight normalization
        std::vector<uint64_t> counts;
        std::transform(colorHistogram.cbegin(), colorHistogram.cend(), std::back_inserter(counts), [](const auto &h)
                       { return h.second; });
        std::nth_element(counts.begin(), counts.begin() + counts.size() / 2, counts.end());
        auto medianIt = counts.cbegin() + counts.size() / 2;
        // now get sizePercent values centered around median
        const std::size_t halfRange = static_cast<std::size_t>(counts.size() / 2.0 * HistogramCountPercent / 100.0);
        // auto leftIt = std::prev(medianIt, halfRange);  // min border of range
        auto rightIt = std::next(medianIt, halfRange); // max border of range
        const auto maxCount = *rightIt;
        // create as many objects as colors in histogram. normalize weight and clamp to [0,1]
        std::map<PIXEL_TYPE, ColorObject> objects;
        std::transform(colorHistogram.cbegin(), colorHistogram.cend(), std::inserter(objects, objects.end()), [maxCount](const auto &entry)
                       { return std::make_pair(entry.first, ColorObject{ColorObject::Type::Outlier, entry.second >= maxCount ? 1.0 : static_cast<double>(static_cast<double>(entry.second) / static_cast<double>(maxCount)), {}, false, InvalidClusterIndex}); });
        std::vector<Cluster> clusters;
        do
        {
            const double allowedNeighbourDistance = 0.5 * (maxNeighbourDistance + minNeighbourDistance);
            // find nearby objects
#pragma omp parallel for
            for (int i = 0; i < static_cast<int>(objects.size()); i++)
            {
                auto objectIt = std::next(objects.begin(), i);
                auto &object = objectIt->second;
                // clear object (from previous loop)
                object.type = ColorObject::Type::Outlier;
                object.neighbours.clear();
                object.visited = false;
                object.clusterIndex = InvalidClusterIndex;
                // find neighbours
                for (auto otherIt = objects.cbegin(); otherIt != objects.cend(); ++otherIt)
                {
                    // ignore same object
                    if (objectIt != otherIt)
                    {
                        // find distance to supposed neighbour
                        const auto objectColor = objectIt->first;
                        const auto otherColor = otherIt->first;
                        if (PIXEL_TYPE::mse(objectColor, otherColor) <= allowedNeighbourDistance)
                        {
                            // other is neighbour. add to object
                            object.neighbours.push_back(otherColor);
                        }
                    }
                }
                // neighbours searched. categorize object
                if (object.neighbours.size() >= MinNeighbourCount)
                {
                    object.type = ColorObject::Type::Core;
                }
                else if (object.neighbours.size() > 0)
                {
                    object.type = ColorObject::Type::NonCore;
                }
            }
            const auto nrOfCoreObject = std::count_if(objects.cbegin(), objects.cend(), [](const auto &object)
                                                      { return object.second.type == ColorObject::Type::Core; });
            const auto nrOfNonCoreObjects = std::count_if(objects.cbegin(), objects.cend(), [](const auto &object)
                                                          { return object.second.type == ColorObject::Type::NonCore; });
            const auto nrOfOutliers = std::count_if(objects.cbegin(), objects.cend(), [](const auto &object)
                                                    { return object.second.type == ColorObject::Type::Outlier; });
            const auto nrOfImportantOutliers = std::count_if(objects.cbegin(), objects.cend(), [](const auto &object)
                                                             { return object.second.type == ColorObject::Type::Outlier && object.second.weight >= MinImportantOutlierWeight; });
            std::cout << nrOfCoreObject << " core objects, " << nrOfNonCoreObjects << " non-core objects, " << nrOfOutliers << " outliers (" << nrOfImportantOutliers << " important)" << std::endl;
            const auto nrOfNeighboursObject = std::accumulate(objects.cbegin(), objects.cend(), std::size_t(0), [](const auto &v, const auto &o)
                                                              { return v + o.second.neighbours.size(); });
            std::cout << nrOfNeighboursObject << " neighbours, ~" << nrOfNeighboursObject / objects.size() << " per object" << std::endl;
            // build clusters from core objects
            clusters.clear();
            for (auto objectIt = objects.begin(); objectIt != objects.end(); ++objectIt)
            {
                auto &object = objectIt->second;
                if (!object.visited)
                {
                    object.visited = true;
                    if (object.type == ColorObject::Type::Core)
                    {
                        // is this object part of a cluster?
                        if (object.clusterIndex == InvalidClusterIndex)
                        {
                            // not yet. create new cluster and add object to it
                            object.clusterIndex = clusters.size();
                            auto objectColor = objectIt->first;
                            clusters.push_back(Cluster{objectColor, 0.0F, {}, objectColor});
                        }
                        auto &cluster = clusters[object.clusterIndex];
                        // add unvisited neighbouring core and reachable points to same cluster
                        for (const auto neighbourColor : object.neighbours)
                        {
                            auto &neighbour = objects[neighbourColor];
                            if (!neighbour.visited && neighbour.clusterIndex == InvalidClusterIndex)
                            {
                                // check if cluster range would be to big if we added this object
                                auto newBounds = cluster.objectBounds | neighbourColor;
                                if (newBounds.size() <= maxClusterDimension)
                                {
                                    // update bounds
                                    cluster.objectBounds = newBounds;
                                    // add object to cluster
                                    neighbour.clusterIndex = object.clusterIndex;
                                    cluster.objects.push_back(neighbourColor);
                                    // if the neigbour was not visited, but is a non-core point, mark it as visited, as we do not need to process it any further
                                    if (neighbour.type == ColorObject::Type::NonCore)
                                    {
                                        neighbour.visited = true;
                                    }
                                }
                            }
                        }
                    }
                }
            }
            // check if we have left-over objects
            for (auto objectIt = objects.begin(); objectIt != objects.end(); ++objectIt)
            {
                auto &object = objectIt->second;
                if (!object.visited)
                {
                    object.visited = true;
                    if (object.type != ColorObject::Type::Outlier && object.clusterIndex == InvalidClusterIndex)
                    {
                        THROW(std::runtime_error, "Unassigned core objects");
                    }
                }
            }
            // calculate cluster centers using cluster color objects and their weights and snap to colorspace colors
#pragma omp parallel for
            for (int i = 0; i < static_cast<int>(clusters.size()); i++)
            {
                Eigen::Vector3f clusterCenter(0, 0, 0);
                double clusterWeight = 0.0;
                // calculate weighted center of cluster
                for (const auto objectCenter : clusters[i].objects)
                {
                    const auto &object = objects[objectCenter];
                    auto combinedWeight = clusterWeight + object.weight;
                    auto t0 = clusterWeight / combinedWeight;
                    auto t1 = 1.0F - t0;
                    clusterCenter.x() = t0 * clusterCenter.x() + t1 * objectCenter.R();
                    clusterCenter.y() = t0 * clusterCenter.y() + t1 * objectCenter.G();
                    clusterCenter.z() = t0 * clusterCenter.z() + t1 * objectCenter.B();
                    clusterWeight = combinedWeight;
                }
                PIXEL_TYPE centerColor(clusterCenter.x(), clusterCenter.y(), clusterCenter.z());
                // snap center to closest colorspace color
                auto snappedColor = getClosestColor(centerColor, m_colorSpace);
#pragma omp critical
                {
                    clusters[i].center = snappedColor;
                    clusters[i].weight = clusterWeight;
                }
            }
            // snap important outliers to closest colorspace color (other outliers will be ignored)
#pragma omp parallel for
            for (int i = 0; i < static_cast<int>(objects.size()); i++)
            {
                auto objectIt = std::next(objects.begin(), i);
                if (objectIt->second.type == ColorObject::Type::Outlier && objectIt->second.weight >= MinImportantOutlierWeight)
                {
                    // snap outliers to closest colorspace color
                    auto snappedColor = getClosestColor(objectIt->first, m_colorSpace);
#pragma omp critical
                    {
                        clusters.push_back({snappedColor, objectIt->second.weight, {}});
                    }
                }
            }
            // dump to file
            dumpToCSV(clusters, objects);
            /*// Check if we can combine some of the outliers that are neighbours to clusters
            std::for_each(objects.begin(), objects.end(), [](auto &o)
                          { o.visited = false; });
            for (auto objectIt = objects.begin(); objectIt != objects.end(); ++objectIt)
            {
                auto &object = objectIt->second;
                if (!object.visited)
                {
                    object.visited = true;
                    if (object.type == ColorObject::Type::Outlier)
                    {
                        // is this object part of a cluster?
                        if (object.clusterIndex == InvalidClusterIndex)
                        {
                            // not yet. create new cluster and add object to it
                            object.clusterIndex = clusters.size();
                            auto objectColor = objectIt->first;
                            clusters.push_back(Cluster{{objectColor}});
                        }
                        // find outliers in
                        for (auto objectIt = objects.begin(); objectIt != objects.end(); ++objectIt)
                        {
                        }
                    }
                }
            }*/
            // count clusters
            const auto nrOfClusters = clusters.size();
            std::cout << nrOfClusters << " clusters" << std::endl;
            if (nrOfClusters < nrOfColors)
            {
                // if we have too few results, reduce the allowed distance
                auto t = static_cast<double>(nrOfColors - nrOfClusters) / nrOfColors;
                t = 0.5 * (t > 1.0 ? 1.0 : t);
                maxNeighbourDistance = (0.5 + t) * allowedNeighbourDistance + (0.5 - t) * maxNeighbourDistance;
            }
            else if (nrOfClusters > nrOfColors)
            {
                // if we have too many results, increase the allowed distance
                auto t = static_cast<double>(nrOfClusters - nrOfColors) / nrOfColors;
                t = 0.5 * (t > 1.0 ? 1.0 : t);
                minNeighbourDistance = (0.5 + t) * allowedNeighbourDistance + (0.5 - t) * minNeighbourDistance;
            }
            else
            {
                break;
            }
        } while (--iterationsLeft > 0);
        REQUIRE(clusters.size() > 0 && clusters.size() <= nrOfColors, std::runtime_error, "Failed to find expected number of clusters");
        // return reduced set of colors and mapping from reduced set of colors to original colors
        std::map<PIXEL_TYPE, std::vector<PIXEL_TYPE>> colorMapping;
        std::transform(clusters.cbegin(), clusters.cend(), std::inserter(colorMapping, colorMapping.end()), [](const auto &c)
                       { return std::make_pair(c.center, c.objects); });
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

    static auto dumpToCSV(const std::vector<Cluster> &clusters, const std::map<PIXEL_TYPE, ColorObject> &objects) -> void
    {
        std::ofstream csvObjects("result/colorfit_objects.csv");
        IO::CSV::writeCSV(csvObjects, {"r", "g", "b", "csscolor", "weight", "type", "clusterindex"}, objects, [](decltype(*objects.cbegin()) o, std::size_t index)
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
                                return std::to_string(o.second.weight);
                            case 5:
                                return std::to_string(static_cast<std::size_t>(o.second.type));
                            case 6:
                                return std::to_string(o.second.clusterIndex >= InvalidClusterIndex ? -10L : static_cast<int64_t>(o.second.clusterIndex));
                            default:
                                THROW(std::runtime_error, "Bad index");
                            } });
        std::ofstream csvClusters("result/colorfit_clusters.csv");
        IO::CSV::writeCSV(csvClusters, {"r", "g", "b", "csscolor"}, clusters, [](decltype(clusters.front()) c, std::size_t index)
                          { switch (index) {
                            case 0:
                                return std::to_string(c.center.R());
                            case 1:
                                return std::to_string(c.center.G());
                            case 2:
                                return std::to_string(c.center.B());
                            case 3:
                                return "#" + c.center.toHex();
                            default:
                                THROW(std::runtime_error, "Bad index");
                            } });
    }

    std::vector<PIXEL_TYPE> m_colorSpace;
};
