#include "color.h"

#include <Eigen/Core>
#include <Eigen/Dense>

namespace Color
{

    YCgCoRd YCgCoRd::fromRGB888(const uint8_t *rgb888)
    {
        YCgCoRd result;
        result.Co = static_cast<double>(rgb888[0]) - static_cast<double>(rgb888[2]);
        double tmp = static_cast<double>(rgb888[2]) + result.Co / 2.0;
        result.Cg = static_cast<double>(rgb888[1]) - tmp;
        result.Y = tmp + static_cast<double>(rgb888[1]) / 2.0;
        return result;
    }

    RGBd fromRGB555(uint16_t color)
    {
        return {static_cast<double>((color & 0x7C00) >> 10) / 31.0, static_cast<double>((color & 0x3E0) >> 5) / 31.0, static_cast<double>(color & 0x1F) / 31.0};
    }

    uint16_t toRGB555(const RGBd &color)
    {
        // bring into range
        auto cr = color.x() * 31.0;
        auto cg = color.y() * 31.0;
        auto cb = color.z() * 31.0;
        // clamp to [0,31]
        cr = cr < 0.0 ? 0.0 : (cr > 31.0 ? 31.0 : cr);
        cg = cg < 0.0 ? 0.0 : (cg > 31.0 ? 31.0 : cg);
        cb = cb < 0.0 ? 0.0 : (cb > 31.0 ? 31.0 : cb);
        // convert to RGB555
        auto r = static_cast<uint16_t>(cr);
        auto g = static_cast<uint16_t>(cg);
        auto b = static_cast<uint16_t>(cb);
        return (r << 10) | (g << 5) | b;
    }

    RGBd roundToRGB555(const RGBd &color)
    {
        // bring into range
        auto cr = color.x() * 31.0;
        auto cg = color.y() * 31.0;
        auto cb = color.z() * 31.0;
        // clamp to [0, GRID_MAX]
        cr = cr < 0.0 ? 0.0 : (cr > 31.0 ? 31.0 : cr);
        cg = cg < 0.0 ? 0.0 : (cg > 31.0 ? 31.0 : cg);
        cb = cb < 0.0 ? 0.0 : (cb > 31.0 ? 31.0 : cb);
        // round to grid point
        cr = std::trunc(cr + 0.5);
        cg = std::trunc(cg + 0.5);
        cb = std::trunc(cb + 0.5);
        return {cr / 31.0, cg / 31.0, cb / 31.0};
    }

    double distance(const RGBd &color0, const RGBd &color1)
    {
        if (color0 == color1)
        {
            return 0.0;
        }
        double ra = color0.x();
        double rb = color1.x();
        double r = 0.5 * (ra + rb);
        double dR = ra - rb;
        double dG = color0.y() - color1.y();
        double dB = color0.z() - color1.z();
        return (2.0 + r) * dR * dR + 4.0 * dG * dG + (3.0 - r) * dB * dB;
    } // max:  (2 + 0.5) *  1 *  1 + 4   *  1 *  1 + (3 - 0.5) *  1 *  1 = 2.5 + 4 + 2.5 = 9

    double distance(const std::array<RGBd, 16> &colors0, const std::array<RGBd, 16> &colors1)
    {
        double dist = 0.0;
        for (auto c0It = colors0.cbegin(), c1It = colors1.cbegin(); c0It != colors0.end(); ++c0It, ++c1It)
        {
            dist += distance(*c0It, *c1It);
        }
        return dist / 16.0;
    }

    std::pair<RGBd, RGBd> lineFit(const std::array<Color::RGBd, 16> &colors)
    {
        // copy coordinates to matrix in Eigen format
        constexpr size_t NrOfAtoms = 16;
        Eigen::Matrix<Color::RGBd::Scalar, Eigen::Dynamic, Eigen::Dynamic> points(3, NrOfAtoms);
        for (size_t i = 0; i < NrOfAtoms; ++i)
        {
            points.col(i) = colors[i];
        }
        // center on mean
        Color::RGBd mean = points.rowwise().mean();
        auto centered = points.colwise() - mean;
        // calculate SVD and first eigenvector
        auto svd = centered.jacobiSvd(Eigen::DecompositionOptions::ComputeFullU);
        Color::RGBd axis = svd.matrixU().col(0).transpose().normalized();
        return {mean, axis};
    }

}