#include "rgb888.h"

#include <iomanip>

namespace Color
{

    auto RGB888::swapToBGR() const -> RGB888
    {
        return RGB888(B(), G(), R());
    }

    auto RGB888::fromHex(const std::string &hex) -> RGB888
    {
        REQUIRE(hex.length() >= 6, std::runtime_error, "Hex color string must have format RRGGBB or #RRGGBB");
        auto temp = hex;
        // remove # if it exists
        if (temp.front() == '#')
        {
            temp = temp.erase(0, 1);
        }
        REQUIRE(temp.length() == 6, std::runtime_error, "Hex color string must have format RRGGBB or #RRGGBB");
        // extract RGB values
        try
        {
            auto R = std::stoi(temp.substr(0, 2), nullptr, 16);
            auto G = std::stoi(temp.substr(2, 2), nullptr, 16);
            auto B = std::stoi(temp.substr(4, 2), nullptr, 16);
            return RGB888(R, G, B);
        }
        catch (const std::invalid_argument &e)
        {
            THROW(std::runtime_error, "Hex color conversion failed: " << e.what());
        }
    }

    auto RGB888::toHex() const -> std::string
    {
        return toHex(*this);
    }

    auto RGB888::toHex(const RGB888 &color) -> std::string
    {
        std::stringstream ss;
        ss << std::uppercase << std::hex << std::setfill('0') << std::setw(2) << static_cast<uint32_t>(color.R()) << std::setw(2) << static_cast<uint32_t>(color.G()) << std::setw(2) << static_cast<uint32_t>(color.B());
        return ss.str();
    }

    auto RGB888::mse(const RGB888 &color0, const RGB888 &color1) -> float
    {
        static constexpr float OneOver255 = 1.0F / 255.0F;
        if (color0.R() == color1.R() && color0.G() == color1.G() && color0.B() == color1.B())
        {
            return 0.0F;
        }
        float ra = static_cast<float>(color0.R()) * OneOver255;
        float rb = static_cast<float>(color1.R()) * OneOver255;
        float rMean = 0.5F * (ra + rb);
        float dR = ra - rb;
        float dG = (static_cast<float>(color0.G()) - static_cast<float>(color1.G())) * OneOver255;
        float dB = (static_cast<float>(color0.B()) - static_cast<float>(color1.B())) * OneOver255;
        return ((2.0F + rMean) * dR * dR + 4.0F * dG * dG + (3.0F - rMean) * dB * dB) / 9.0F;
    } // max:   (2    +     1) *  1 *  1 + 4    *  1 *  1 + (3    -     1) *  1 *  1 = 3 + 4 + 2 = 9 / 9 = 1

    bool operator==(const RGB888 &c1, const RGB888 &c2)
    {
        return c1.v == c2.v;
    }

    bool operator!=(const RGB888 &c1, const RGB888 &c2)
    {
        return !(c1 == c2);
    }
}