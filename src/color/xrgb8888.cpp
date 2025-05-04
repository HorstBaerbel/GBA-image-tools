#include "xrgb8888.h"

#include <iomanip>

namespace Color
{

    auto XRGB8888::swapToBGR() const -> XRGB8888
    {
        return XRGB8888(B(), G(), R());
    }

    auto XRGB8888::fromHex(const std::string &hex) -> XRGB8888
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
            return XRGB8888(R, G, B);
        }
        catch (const std::invalid_argument &e)
        {
            THROW(std::runtime_error, "Hex color conversion failed: " << e.what());
        }
    }

    auto XRGB8888::toHex() const -> std::string
    {
        return toHex(*this);
    }

    auto XRGB8888::toHex(const XRGB8888 &color) -> std::string
    {
        std::stringstream ss;
        ss << std::uppercase << std::hex << std::setfill('0') << std::setw(2) << static_cast<uint32_t>(color.R()) << std::setw(2) << static_cast<uint32_t>(color.G()) << std::setw(2) << static_cast<uint32_t>(color.B());
        return ss.str();
    }

    bool operator==(const XRGB8888 &c1, const XRGB8888 &c2)
    {
        return c1.v == c2.v;
    }

    bool operator!=(const XRGB8888 &c1, const XRGB8888 &c2)
    {
        return !(c1 == c2);
    }
}