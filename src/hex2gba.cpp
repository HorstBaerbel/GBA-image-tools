// Convert a RGB888 color value to RGB555, RGB565 and BGR555, BGR565 for GBA / NDS
// See also: http://www.budmelvin.com/dev/15bitconverter.html
// and: https://en.wikipedia.org/wiki/High_color

#include <iomanip>
#include <iostream>
#include <string>

#include <cxxopts/include/cxxopts.hpp>

#include "color/xrgb8888.h"

Color::XRGB8888 m_color;

bool readArguments(int argc, const char *argv[])
{
    cxxopts::Options options("hex2gba", "Convert a RGB888 color value to RGB555, RGB565 and BGR555, BGR565 for GBA / NDS");
    options.allow_unrecognised_options();
    options.add_options()("h,help", "Print help")("c,color", "Color must be a RGB888 hex value like \"abc012\" or \"#abc012\"", cxxopts::value<std::string>())("positional", "", cxxopts::value<std::vector<std::string>>());
    options.parse_positional({"color", "positional"});
    auto result = options.parse(argc, argv);
    // check if help was requested
    if (result.count("h"))
    {
        return false;
    }
    // color value
    if (result.count("color"))
    {
        // try converting argument to a color
        auto colorString = result["color"].as<std::string>();
        try
        {
            m_color = Color::XRGB8888::fromHex(colorString);
        }
        catch (const std::runtime_error &e)
        {
            std::cerr << colorString << " is not a valid color: " << e.what() << ". Format must be \"abc012\" or \"#abc012\". Aborting. " << std::endl;
            return false;
        }
    }
    return true;
}

void printUsage()
{
    std::cout << "Convert a RGB888 color value to RGB555, RGB565 and BGR555, BGR565 for GBA / NDS" << std::endl;
    std::cout << "Usage: hex2gba COLOR" << std::endl;
    std::cout << "COLOR must be a RGB888 hex value like \"abc012\" or \"#abc012\"" << std::endl;
}

auto printColor(const std::string &format, uint32_t color32, uint32_t color16) -> void
{
    std::cout << format << std::hex << std::setfill('0') << std::setw(4) << color32 << ", ";
    std::cout << std::hex << std::setfill('0') << std::setw(4) << color16 << "h, ";
    std::cout << std::dec << color16 << "d" << std::endl;
}

int main(int argc, const char *argv[])
{
    // check arguments
    if (argc < 2 || !readArguments(argc, argv))
    {
        printUsage();
        return 2;
    }
    // convert color
    auto b5 = static_cast<uint32_t>(m_color.R() >> 3);
    auto g5 = static_cast<uint32_t>(m_color.G() >> 3);
    auto g6 = static_cast<uint32_t>(m_color.G() >> 2);
    auto r5 = static_cast<uint32_t>(m_color.B() >> 3);
    printColor("RGB555 = #", (b5 << 19) | (g5 << 11) | (r5 << 3), (b5 << 10) | (g5 << 5) | r5);
    printColor("BGR555 = #", (r5 << 19) | (g5 << 11) | (b5 << 3), (r5 << 10) | (g5 << 5) | b5);
    printColor("RGB565 = #", (b5 << 19) | (g6 << 10) | (r5 << 3), (b5 << 11) | (g6 << 5) | r5);
    printColor("BGR565 = #", (r5 << 19) | (g6 << 10) | (b5 << 3), (r5 << 11) | (g6 << 5) | b5);
    return 0;
}
