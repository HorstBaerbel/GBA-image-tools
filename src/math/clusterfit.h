#pragma once

#include <vector>

/// @brief Fit points to clusters using k-means
/// @tparam T Value or struct type
/// @tparam Distance Function type that can calculate the distance between two Ts
/// @param p Input points
/// @param nrOfClusters Number of clusters to use
/// Found here: https://en.wikipedia.org/wiki/K-means_clustering
/// See also: https://www.goldsborough.me/c++/python/cuda/2017/09/10/20-32-46-exploring_k-means_in_python,_c++_and_cuda/
/// @return Returns nrOfClusters cluster central points
template <typename T, class Distance>
auto clusterFit(const std::vector<T> &p, Distance distance, std::size_t nrOfClusters) -> std::vector<T>
{
    // copy coordinates to matrix in Eigen format
    constexpr std::size_t NrOfAtoms = N;
    Eigen::Matrix<double, Eigen::Dynamic, Eigen::Dynamic> points(3, NrOfAtoms);
    for (std::size_t i = 0; i < NrOfAtoms; ++i)
    {
        points.col(i) = p[i];
    }
    // center on mean
    Eigen::Vector3d mean = points.rowwise().mean();
    auto centered = points.colwise() - mean;
    // calculate SVD and first eigenvector
    auto svd = centered.jacobiSvd(Eigen::DecompositionOptions::ComputeFullU);
    Eigen::Vector3d axis = svd.matrixU().col(0).transpose().normalized();
    return {T(mean), T(axis)};
}
