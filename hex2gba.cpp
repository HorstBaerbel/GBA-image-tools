// Convert a RGB888 color value to RGB555 for GBA
// See also: http://www.budmelvin.com/dev/15bitconverter.html
// and: https://en.wikipedia.org/wiki/High_color

#include <iostream>
#include <iomanip>
#include <string>

#include "cxxopts/include/cxxopts.hpp"
#include <Magick++.h>

using namespace Magick;

Color m_color;

bool readArguments(int argc, const char *argv[])
{
    cxxopts::Options options("hextogba", "Convert a RGB888 color value to RGB555 and BGR555 for GBA");
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
        auto colorArg = result["color"].as<std::string>();
        auto colorString = colorArg.front() == '#' ? colorArg : (std::string("#") + colorArg);
        try
        {
            m_color = Color(colorString);
        }
        catch (const Exception &ex)
        {
            std::cerr << colorArg << " is not a valid color. Format must be \"abc012\" or \"#abc012\". Aborting. " << std::endl;
            return false;
        }
    }
    return true;
}

void printUsage()
{
    std::cout << "Convert a RGB888 color value to RGB555 and BGR555 for GBA" << std::endl;
    std::cout << "Usage: hex2gba COLOR" << std::endl;
    std::cout << "COLOR must be a RGB888 hex value like \"abc012\" or \"#abc012\"" << std::endl;
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
    uint16_t r = static_cast<uint16_t>((31 * static_cast<uint32_t>(m_color.redQuantum())) / static_cast<uint32_t>(QuantumRange));
    uint16_t g = static_cast<uint16_t>((31 * static_cast<uint32_t>(m_color.greenQuantum())) / static_cast<uint32_t>(QuantumRange));
    uint16_t b = static_cast<uint16_t>((31 * static_cast<uint32_t>(m_color.blueQuantum())) / static_cast<uint32_t>(QuantumRange));
    uint16_t rgb = (r << 10 | g << 5 | b);
    std::cout << "RGB555 = #" << std::hex << std::setfill('0') << std::setw(4) << rgb << ", ";
    std::cout << std::hex << std::setfill('0') << std::setw(4) << rgb << "h, ";
    std::cout << std::dec << rgb << "d" << std::endl;
    uint16_t bgr = (b << 10 | g << 5 | r);
    std::cout << "BGR555 = #" << std::hex << std::setfill('0') << std::setw(4) << bgr << ", ";
    std::cout << std::hex << std::setfill('0') << std::setw(4) << bgr << "h, ";
    std::cout << std::dec << bgr << "d" << std::endl;
    return 0;
}
