#pragma once

#include <Eigen/Core>
#include <Eigen/Dense>

#include <array>

/// @brief Fit a line through points passed using SVD
/// @tparam T Value or struct type
/// @tparam N Dimension of value block
/// Found here: https://stackoverflow.com/questions/40589802/eigen-best-fit-of-a-plane-to-n-points
/// See also: https://zalo.github.io/blog/line-fitting/
/// See also: https://stackoverflow.com/questions/39370370/eigen-and-svd-to-find-best-fitting-plane-given-a-set-of-points
/// See also: https://gist.github.com/ialhashim/0a2554076a6cf32831ca
/// @return Returns line (origin, axis)
template <typename T, std::size_t N>
auto lineFit(const std::array<T, N> &p) -> std::pair<T, T>
{
    // copy coordinates to matrix in Eigen format
    constexpr std::size_t NrOfAtoms = N;
    Eigen::Matrix<double, Eigen::Dynamic, Eigen::Dynamic> points(3, NrOfAtoms);
    for (std::size_t i = 0; i < NrOfAtoms; ++i)
    {
        points.col(i) = Eigen::Vector3d(p[i].x(), p[i].y(), p[i].z());
    }
    // center on mean
    Eigen::Vector3d mean = points.rowwise().mean();
    auto centered = points.colwise() - mean;
    // calculate SVD and first eigenvector
    Eigen::JacobiSVD<Eigen::MatrixXd, Eigen::DecompositionOptions::ComputeFullU> svd;
    auto fullU = svd.compute(centered);
    Eigen::Vector3d axis = fullU.matrixU().col(0).transpose().normalized();
    return {T(mean.x(), mean.y(), mean.z()), T(axis.x(), axis.y(), axis.z())};
}
