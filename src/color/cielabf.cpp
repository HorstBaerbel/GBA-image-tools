#include "cielabf.h"

#include <Eigen/Core>
#include <Eigen/Dense>

#define DIST_HYAB // Looks subjectively better. See: https://en.wikipedia.org/wiki/Color_difference#Other_geometric_constructions and http://markfairchild.org/PDFs/PAP40.pdf

namespace Color
{
    auto CIELabf::mse(const CIELabf &color0, const CIELabf &color1) -> float
    {
        if (color0 == color1)
        {
            return 0.0F;
        }
        float dL = (color0.L() - color1.L()) / 100.0F; // [-1,1]
        float da = (color0.a() - color1.a()) / 255.0F; // [-1,1]
        float db = (color0.b() - color1.b()) / 255.0F; // [-1,1]
#ifdef DIST_HYAB
        return (std::sqrtf(da * da + db * db) + std::abs(dL)) / 2.414213562F;
    } // max:        (sqrt( 1 *  1 +  1 *  1) +           1)) / (sqrt(2) + 1) = 1
#else
        return (dL * dL + da * da + db * db) / 3.0F;
    } // max:  ( 1 *  1 +  1 *  1 +  1 *  1) / 3 = 1
#endif

    /// See: https://github.com/michel-leonard/ciede2000-color-matching
    auto CIELabf::ciede2000(const CIELabf &color0, const CIELabf &color1) -> float
    {
        // k_l, k_c, k_h are parametric factors to be adjusted according to different viewing parameters such as textures, backgrounds...
        const float k_l = 1.0F;
        const float k_c = 1.0F;
        const float k_h = 1.0F;
        float n = (std::hypot(color0.a(), color0.b()) + std::hypot(color1.a(), color1.b())) * 0.5F;
        n = n * n * n * n * n * n * n;
        n = 1.0F + 0.5F * (1.0F - std::sqrt(n / (n + 6103515625.0F)));
        // hypot calculates the Euclidean distance while avoiding overflow/underflow.
        const float c0 = std::hypot(color0.a() * n, color0.b()), c1 = std::hypot(color1.a() * n, color1.b());
        // atan2 is preferred over atan because it accurately computes the angle of a point (x, y) in all quadrants, handling the signs of both coordinates.
        float h0 = std::atan2(color0.b(), color0.a() * n), h1 = std::atan2(color1.b(), color1.a() * n);
        h0 += 2.0F * M_PI * (h0 < 0.0F);
        h1 += 2.0F * M_PI * (h1 < 0.0F);
        n = std::fabs(h1 - h0);
        // Cross-implementation consistent rounding.
        if (M_PI - 1E-14 < n && n < M_PI + 1E-14)
        {
            n = M_PI;
        }
        // When the hue angles lie in different quadrants, the straightforward average can produce a mean that incorrectly suggests a hue angle in the wrong quadrant, the next lines handle this issue.
        float h_m = (h0 + h1) * 0.5F;
        float h_d = (h1 - h0) * 0.5F;
        h_d += (M_PI < n) * ((0.0F < h_d) - (h_d <= 0.0F)) * M_PI;
        // Some implementations delete the next line and uncomment the one after it, this can lead to a discrepancy of ±0.0003 in the final color difference.
        h_m += (M_PI < n) * M_PI;
        // h_m += (M_PI < n) * ((h_m < M_PI) - (M_PI <= h_m)) * M_PI;
        const float p = 36.0F * h_m - 55.0F * M_PI;
        n = (c0 + c1) * 0.5F;
        n = n * n * n * n * n * n * n;
        // The hue rotation correction term is designed to account for the non-linear behavior of hue differences in the blue region.
        const float r_t = -2.0F * std::sqrt(n / (n + 6103515625.0F)) * std::sin(M_PI / 3.0F * std::exp(p * p / (-25.0F * M_PI * M_PI)));
        n = (color0.L() + color1.L()) * 0.5F;
        n = (n - 50.0F) * (n - 50.0F);
        // Lightness.
        const float l = (color1.L() - color0.L()) / (k_l * (1.0F + 0.015 * n / std::sqrt(20.0F + n)));
        // These coefficients adjust the impact of different harmonic components on the hue difference calculation.
        const float t = 1.0F + 0.24F * std::sin(2.0F * h_m + M_PI * 0.5F) + 0.32F * std::sin(3.0F * h_m + 8.0F * M_PI / 15.0F) - 0.17F * std::sin(h_m + M_PI / 3.0F) - 0.20 * std::sin(4.0F * h_m + 3.0F * M_PI / 20.0F);
        n = c0 + c1;
        // Hue.
        const float h = 2.0F * std::sqrt(c0 * c1) * std::sin(h_d) / (k_h * (1.0F + 0.0075F * n * t));
        // Chroma.
        const float c = (c1 - c0) / (k_c * (1.0F + 0.0225F * n));
        // Returning the square root ensures that the result reflects the actual geometric
        // distance within the color space, which ranges from 0 to approximately 185.
        return std::sqrt(l * l + h * h + c * c + c * h * r_t);
    }
}