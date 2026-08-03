// Generates a paletted image and a table that maps two 8-bit color table entries in the form (i0 << 8 | i1) to the resulting color table entry
// The component calculation is ((source - f) + target) / 2 with rounding.
// This can be used for flowfields and the like

#include "color/colorformat.h"
#include "color/colorhelpers.h"
#include "color/xrgb8888.h"
#include "image/imageio.h"
#include "math/colorfit.h"
#include "image/quantization.h"
#include "image/quantizationmethod.h"

#include <iostream>
#include <fstream>

#include <cxxopts/include/cxxopts.hpp>

std::string m_inFile;
std::string m_outFile;
Color::Format m_outformat = Color::Format::Unknown;
uint32_t m_imgcolors = 0;
uint32_t m_outcolors = 0;
Image::Quantization::Method m_quantizationMethod = Image::Quantization::Method::ClosestColor;
bool m_dryRun = false;

std::string getCommandLine(int argc, const char *argv[])
{
    std::string result;
    for (int i = 1; i < argc; i++)
    {
        result += std::string(argv[i]);
        if (i < (argc - 1))
        {
            result += " ";
        }
    }
    return result;
}

bool readArguments(int argc, const char *argv[])
{
    try
    {
        cxxopts::Options opts("genfadetable", "Generate a paletted image and fade table in RGB555, RGB565 and BGR555, BGR565 for GBA / NDS");
        opts.add_option("", {"h,help", "Print help"});
        opts.add_option("", {"imgcolors", "Number of colors reserved for image, e.g. \"imgcolors=16\" in [1,255]", cxxopts::value<uint32_t>()});
        opts.add_option("", {"outcolors", "Number of output colors / color map size, e.g. \"outcolors=128\" in [1,255]", cxxopts::value<uint32_t>()});
        opts.add_option("", {"outformat", "Set output color format (direct pixel color / color map) to RGB565, RGB555, BGR565 or BGR555", cxxopts::value<std::string>()});
        opts.add_option("", {"infile", "Input file, e.g. \"foo.png\"", cxxopts::value<std::string>()});
        opts.add_option("", {"outname", "Output file and variable name, e.g \"foo\". This will name the output files \"foo.h\" and \"foo.c\" and variable names will start with \"FOO_\"", cxxopts::value<std::string>()});
        opts.parse_positional({"infile", "outname"});
        auto result = opts.parse(argc, argv);
        // check if help was requested
        if (result.count("h"))
        {
            return false;
        }
        // get output file / name
        if (result.count("outname"))
        {
            m_outFile = result["outname"].as<std::string>();
        }
        // get input file(s)
        if (result.count("infile"))
        {
            m_inFile = result["infile"].as<std::string>();
            // make sure input file exist
            if (!std::filesystem::exists(m_inFile))
            {
                std::cout << "Input file \"" << m_inFile << "\" does not exist!" << std::endl;
                return false;
            }
        }
        else
        {
            std::cout << "No input file passed!" << std::endl;
            return false;
        }
        // check output format
        const std::string format = result["outformat"].as<std::string>();
        std::string formatUpper;
        std::transform(format.cbegin(), format.cend(), std::back_inserter(formatUpper),
                       [](unsigned char c)
                       { return std::toupper(c); });
        if (formatUpper == "RGB565")
        {
            m_outformat = Color::Format::RGB565;
        }
        else if (formatUpper == "RGB555")
        {
            m_outformat = Color::Format::XRGB1555;
        }
        else if (formatUpper == "BGR565")
        {
            m_outformat = Color::Format::BGR565;
        }
        else if (formatUpper == "BGR555")
        {
            m_outformat = Color::Format::XBGR1555;
        }
        else
        {
            THROW(std::runtime_error, "Output color format must be RGB565, RGB555, BGR565 or BGR555");
        }
        // check nr of image colors
        m_imgcolors = result["imgcolors"].as<uint32_t>();
        REQUIRE(m_imgcolors > 0 && m_imgcolors < 256, std::runtime_error, "Number of image colors must be in [1,255]");
        // check nr of output colors
        m_outcolors = result["outcolors"].as<uint32_t>();
        REQUIRE(m_outcolors > 0 && m_outcolors < 256, std::runtime_error, "Number of output colors must be in [1,255]");
        REQUIRE(m_outcolors >= m_imgcolors, std::runtime_error, "outcolors must be >= imgcolors");
    }
    catch (const cxxopts::exceptions::parsing &e)
    {
        std::cerr << "In command line: " << getCommandLine(argc, argv) << std::endl;
        std::cerr << "Argument error: " << e.what() << std::endl;
        return false;
    }
    return true;
}

void printUsage()
{
    std::cout << "Generate a paletted image and fade table in RGB555, RGB565 and BGR555, BGR565 for GBA / NDS" << std::endl;
    std::cout << "Usage: genfadetable --outformat=F --imgcolors=M --outcolors=N INFILE OUTNAME" << std::endl;
    std::cout << "outformat: Final image output format F (must be RGB565, RGB555, BGR565, BGR555)" << std::endl;
    std::cout << "imgcolors: Number of colors reserved for image M (must be in [1,255])" << std::endl;
    std::cout << "outcolors: Number of final output colors / map size N (must be in [1,255])" << std::endl;
    std::cout << "INFILE: Input image will be converted to RGB888" << std::endl;
    std::cout << "OUTNAME: Can be an absolute or relative file path or a file base name. Two files" << std::endl;
    std::cout << "outname.h and outname.c will be generated. All variables will begin with the" << std::endl;
    std::cout << "base name portion of OUTNAME." << std::endl;
    std::cout << "help: Show this help." << std::endl;
}

auto buildFadeTable(const uint16_t *palette, uint32_t length, int32_t fadeAmount = 0) -> std::pair<std::vector<uint16_t>, std::vector<uint8_t>>
{
    REQUIRE(length <= 16, std::runtime_error, "Only <= 16 colors allowed");
    // add initial colors to queue of colors to process
    std::deque<uint16_t> colorQueue;
    for (uint32_t i = 0; i < length; ++i)
    {
        colorQueue.push_back(palette[i]);
    }
    // add initial colors to color map
    std::map<uint16_t, uint32_t> colorsAndCounts;
    for (uint32_t i = 0; i < length; ++i)
    {
        colorsAndCounts[palette[i]] = INT32_MAX;
    }
    // generate all possible combinations of mixed colors
    // we generate new colors by:
    // - fade color0 rgb by fadeamount
    // - mix with color1 by adding color0 + color1 rgb and dividing by 2 with rounding
    // if we generate a colors that has not been seen yet, add it to the queue
    // loop until we have not added new colors / the queue is empty
    while (!colorQueue.empty())
    {
        // pop first color from queue
        const uint32_t color0 = colorQueue.front();
        colorQueue.pop_front();
        // check if color already in color map
        auto c0It = colorsAndCounts.find(color0);
        if (c0It == colorsAndCounts.end())
        {
            // no. add to end of colors as we need to mix it with itself too
            colorsAndCounts[color0] = 1;
        }
        // get color components
        auto b0 = (color0 & 0x1F);
        auto g0 = ((color0 & 0x3E0) >> 5);
        auto r0 = ((color0 & 0x7C00) >> 10);
        // fade color by amount
        b0 = b0 > fadeAmount ? b0 - fadeAmount : 0;
        g0 = g0 > fadeAmount ? g0 - fadeAmount : 0;
        r0 = r0 > fadeAmount ? r0 - fadeAmount : 0;
        // mix with other colors
        for (auto c1It = colorsAndCounts.begin(); c1It != colorsAndCounts.end(); ++c1It)
        {
            uint32_t color1 = c1It->first;
            auto b1 = (color1 & 0x1F);
            auto g1 = ((color1 & 0x3E0) >> 5);
            auto r1 = ((color1 & 0x7C00) >> 10);
            // add c0 and c1 with rounding and divide by 2
            auto b = ((b0 << 1) + (b1 << 1) + 1) >> 2;
            auto g = ((g0 << 1) + (g1 << 1) + 1) >> 2;
            auto r = ((r0 << 1) + (r1 << 1) + 1) >> 2;
            auto color = (r << 10) | (g << 5) | b;
            // check if color already exists in map
            auto cIt = colorsAndCounts.find(color);
            if (cIt != colorsAndCounts.end())
            {
                // increase color count
                cIt->second++;
            }
            else if (std::find(colorQueue.cbegin(), colorQueue.cend(), color) == colorQueue.cend())
            {
                // add new color to queue if it is now already in the queue
                colorQueue.push_back(color);
            }
        }
    }
    // now we have all colors that will be generated
    std::vector<uint16_t> sortedColors;
    // sort colors by importance and get the most important 256 colors
    std::transform(colorsAndCounts.cbegin(), colorsAndCounts.cend(), std::back_inserter(sortedColors), [](const auto &v)
                   { return v.first; });
    std::sort(sortedColors.begin(), sortedColors.end(), [&colorsAndCounts](auto a, auto b)
              { return colorsAndCounts[a] > colorsAndCounts[b]; });
    // make sure out original colors are at the front
    std::vector<uint16_t> colorMap;
    for (uint32_t i = 0; i < length; ++i)
    {
        colorMap.push_back(palette[i]);
    }
    // add other colors to a max of 256 colors
    for (auto cIt = sortedColors.cbegin(); cIt != sortedColors.cend() && colorMap.size() < 256; ++cIt)
    {
        // add color only if not already in color map
        if (std::find(colorMap.cbegin(), colorMap.cend(), *cIt) == colorMap.cend())
        {
            colorMap.push_back(*cIt);
        }
    }
    // build map that maps two input colors to one output color
    std::map<uint16_t, uint8_t> fadeMap;
    for (uint32_t index0 = 0; index0 < colorMap.size(); ++index0)
    {
        const uint32_t color0 = colorMap[index0];
        // get color components
        auto b0 = (color0 & 0x1F);
        auto g0 = ((color0 & 0x3E0) >> 5);
        auto r0 = ((color0 & 0x7C00) >> 10);
        // fade color by amount
        b0 = b0 > fadeAmount ? b0 - fadeAmount : 0;
        g0 = g0 > fadeAmount ? g0 - fadeAmount : 0;
        r0 = r0 > fadeAmount ? r0 - fadeAmount : 0;
        for (uint32_t index1 = 0; index1 < colorMap.size(); ++index1)
        {
            const uint32_t color1 = colorMap[index1];
            auto b1 = (color1 & 0x1F);
            auto g1 = ((color1 & 0x3E0) >> 5);
            auto r1 = ((color1 & 0x7C00) >> 10);
            // add c0 and c1 with rounding and divide by 2
            auto b = ((b0 << 1) + (b1 << 1) + 1) >> 2;
            auto g = ((g0 << 1) + (g1 << 1) + 1) >> 2;
            auto r = ((r0 << 1) + (r1 << 1) + 1) >> 2;
            auto newColor = (r << 10) | (g << 5) | b;
            // find out which color is closest to the resulting color
            float bestMSE = 10.0F;
            uint8_t bestIndex = 0;
            for (uint32_t searchIndex = 0; searchIndex < colorMap.size(); ++searchIndex)
            {
                auto colorMSE = mse_rgb555(newColor, colorMap[searchIndex]);
                if (bestMSE > colorMSE)
                {
                    bestMSE = colorMSE;
                    bestIndex = searchIndex;
                }
            }
            // store new color index
            auto colorsHash = (static_cast<uint16_t>(index0) << 8) | static_cast<uint16_t>(index1);
            fadeMap[colorsHash] = bestIndex;
        }
    }
    // fill index map for the whole of the 16-bit space
    std::vector<uint8_t> indexMap(65536, 0);
    for (const auto &fadeEntry : fadeMap)
    {
        indexMap[fadeEntry.first] = fadeEntry.second;
    }
    return {colorMap, indexMap};
}

int main(int argc, char *argv[])
{

    try
    {
        // check arguments
        if (argc < 3 || !readArguments(argc, argv))
        {
            printUsage();
            return 2;
        }
        // check input and output
        if (m_inFile.empty())
        {
            std::cerr << "No input file(s) passed. Aborting." << std::endl;
            return 1;
        }
        if (m_outFile.empty())
        {
            std::cerr << "No output file passed. Aborting." << std::endl;
            return 1;
        }
        // set up number of cores for parallel processing
        const auto nrOfProcessors = omp_get_num_procs();
        omp_set_num_threads(nrOfProcessors);
        // read image
        std::cout << "Reading " << m_inFile;
        Image::Frame img;
        try
        {
            img = IO::File::readImage(m_inFile);
        }
        catch (const std::runtime_error &e)
        {
            THROW(std::runtime_error, "Failed to read image: " << e.what());
        }
        const auto imgSize = img.info.size;
        std::cout << " -> " << imgSize.width() << "x" << imgSize.height() << ", ";
        const auto imgFormat = img.data.pixels().format();
        std::cout << Color::formatInfo(imgFormat).name;
        const auto imgIsIndexed = img.data.pixels().isIndexed();
        std::cout << std::endl;
        // add palette conversion using a RGB555 or RGB565 reference color map
        std::vector<Color::XRGB8888> colorSpaceMap;
        switch (m_outformat)
        {
        case Color::Format::XBGR1555:
            colorSpaceMap = ColorHelpers::buildColorMapFor(Color::Format::XRGB1555);
            break;
        case Color::Format::BGR565:
            colorSpaceMap = ColorHelpers::buildColorMapFor(Color::Format::RGB565);
            break;
        default:
            colorSpaceMap = ColorHelpers::buildColorMapFor(m_outformat);
        }
        // ----- convert output image to paletted -----
        // use cluster fit to find optimum color mapping
        ColorFit<Color::XRGB8888> colorFit(colorSpaceMap);
        const auto srcPixels = img.data.pixels().data<Color::XRGB8888>();
        const auto colorMapping = colorFit.reduceColors(srcPixels, m_imgcolors);
        REQUIRE(colorMapping.size() > 0 && m_imgcolors >= colorMapping.size(), std::runtime_error, "Unexpected number of mapped colors");
        // convert image to paletted possibly using dithering
        Image::ImageData palettedImg;
        switch (m_quantizationMethod)
        {
        case Image::Quantization::Method::ClosestColor:
            palettedImg = Image::Quantization::quantizeClosest(img.data, colorMapping);
            break;
        case Image::Quantization::Method::AtkinsonDither:
            palettedImg = Image::Quantization::atkinsonDither(img.data, img.info.size.width(), img.info.size.height(), colorMapping);
            break;
        default:
            THROW(std::runtime_error, "Unsupported quantization method " << Image::Quantization::toString(m_quantizationMethod));
        }
        REQUIRE(palettedImg.pixels().format() == Color::Format::Paletted8, std::runtime_error, "Expected 8-bit paletted return image");
        // ----- create faded table -----
        // write output images
        if (!m_dryRun)
        {
            std::ofstream hFile(m_outFile + ".h", std::ios::out);
            std::ofstream cFile(m_outFile + ".c", std::ios::out);
            if (hFile.is_open() && cFile.is_open())
            {
                std::cout << "Writing output files " << m_outFile << ".h, " << m_outFile << ".c" << std::endl;
                try
                {
                    // build output file / variable name
                    std::string baseName = std::filesystem::path(m_outFile).filename().replace_extension("");
                    std::string varName = baseName;
                    std::transform(varName.begin(), varName.end(), varName.begin(), [](char c)
                                   { return std::toupper(c, std::locale()); });
                    // output header
                    hFile << "// Converted with img2h " << getCommandLine(argc, argv) << std::endl;
                    hFile << "// Note that the _Alignas specifier will need C11, as a workaround use __attribute__((aligned(4)))" << std::endl
                          << std::endl;
                    // output data info
                    hFile << "// Data is";
                    if (data0.type.isBitmap())
                    {
                        hFile << " bitmap";
                    }
                    if (data0.type.isSprites())
                    {
                        hFile << " sprites";
                    }
                    if (data0.type.isTiles() && !data0.map.data.empty())
                    {
                        hFile << " tilemap";
                    }
                    else
                    {
                        hFile << " tiles";
                    }
                    if (data0.type.isCompressed())
                    {
                        hFile << " compressed";
                    }
                    hFile << ", pixel format: " << Color::formatInfo(data0.info.pixelFormat).name;
                    if (data0.info.pixelFormat != Color::Format::Unknown)
                    {
                        hFile << ", color map format: " << Color::formatInfo(data0.info.colorMapFormat).name;
                    }
                    hFile << std::endl
                          << std::endl;
                }
            }
        }
        catch (const std::runtime_error &e)
        {
            std::cerr << "Error: " << e.what() << std::endl;
            return 1;
        }
        return 0;
    }