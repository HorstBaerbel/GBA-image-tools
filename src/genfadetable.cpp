// Generates a paletted image and a table that maps two 8-bit color table entries in the form (i0 << 8 | i1) to the resulting color table entry
// The component calculation is ((source - f) + target) / 2 with rounding.
// This can be used for flowfields and the like

#include "color/colorformat.h"
#include "color/colorhelpers.h"
#include "color/xrgb8888.h"
#include "image/imagedatahelpers.h"
#include "image/imageio.h"
#include "image/quantization.h"
#include "image/quantizationmethod.h"
#include "io/textio.h"
#include "math/colorfit.h"

#include <fstream>
#include <iostream>
#include <type_traits>

#include "cxxopts/include/cxxopts.hpp"
#include "glob/single_include/glob/glob.hpp"

std::vector<std::string> m_inFile;
std::string m_outFile;
Color::Format m_outformat = Color::Format::Unknown;
uint32_t m_imgcolors = 0;
uint32_t m_outcolors = 0;
uint32_t m_fadeAmount = 0;
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
        cxxopts::Options opts("genfadetable", "Generate paletted image(s) and fade table in RGB555, RGB565 and BGR555, BGR565 for GBA / NDS");
        opts.add_option("", {"h,help", "Print help"});
        opts.add_option("", {"imgcolors", "Number of colors reserved for image, e.g. \"imgcolors=16\" in [1,255]", cxxopts::value<uint32_t>()});
        opts.add_option("", {"outcolors", "Number of output colors / color map size, e.g. \"outcolors=128\" in [1,255]", cxxopts::value<uint32_t>()});
        opts.add_option("", {"outformat", "Set output color format (direct pixel color / color map) to RGB565, RGB555, BGR565 or BGR555", cxxopts::value<std::string>()});
        opts.add_option("", {"fadeamount", "How much to fade per step, e.g. \"fadeamount=2\" in [1,8]", cxxopts::value<uint32_t>()});
        opts.add_option("", {"infile", "Input file(s), e.g. \"foo.png\"", cxxopts::value<std::vector<std::string>>()});
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
            m_inFile = result["infile"].as<std::vector<std::string>>();
            // get last positional argument as output file / name
            if (m_outFile.empty())
            {
                m_outFile = m_inFile.back();
                m_inFile.resize(m_inFile.size() - 1);
            }
            // check if the name contains possible wildcard characters
            if (m_inFile.size() == 1 && m_inFile.front().find_first_of("*?![]") != std::string::npos)
            {
                // resolve wildcards in input files
                auto filePaths = glob::glob(m_inFile);
                m_inFile.clear();
                std::transform(filePaths.cbegin(), filePaths.cend(), std::back_inserter(m_inFile), [](const auto &p)
                               { return p.string(); });
            }
            // make sure all input files exist
            for (const auto &fileName : m_inFile)
            {
                if (!std::filesystem::exists(fileName))
                {
                    std::cout << "Input file \"" << fileName << "\" does not exist!" << std::endl;
                    return false;
                }
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
        REQUIRE(m_imgcolors > 0 && m_imgcolors <= 256, std::runtime_error, "Number of image colors must be in [1,256]");
        // check nr of output colors
        m_outcolors = result["outcolors"].as<uint32_t>();
        REQUIRE(m_outcolors > 0 && m_outcolors <= 256, std::runtime_error, "Number of output colors must be in [1,256]");
        REQUIRE(m_outcolors >= m_imgcolors, std::runtime_error, "outcolors must be >= imgcolors");
        // check fade amount
        m_fadeAmount = result["fadeamount"].as<uint32_t>();
        REQUIRE(m_fadeAmount > 0 && m_fadeAmount <= 8, std::runtime_error, "Fade amount must be in [1,8]");
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
    std::cout << "---------> Number of fade colors = N - M" << std::endl;
    std::cout << "fadeamount: How much to fade per step (must be in [1,8])" << std::endl;
    std::cout << "INFILE: Input image will be converted to RGB888" << std::endl;
    std::cout << "OUTNAME: Can be an absolute or relative file path or a file base name. Two files" << std::endl;
    std::cout << "outname.h and outname.c will be generated. All variables will begin with the" << std::endl;
    std::cout << "base name portion of OUTNAME." << std::endl;
    std::cout << "help: Show this help." << std::endl;
}

template <typename OUT_FORMAT>
auto fadeColor(Color::XRGB8888 color0, Color::XRGB8888 color1, const uint32_t fadeAmount)
{
    constexpr uint32_t ShiftRB = 3;
    constexpr uint32_t ShiftG = std::is_same_v<OUT_FORMAT, Color::RGB565> ? 2 : 3;
    const int32_t FadeAmountRB = fadeAmount * (1 << ShiftRB);
    const int32_t FadeAmountG = fadeAmount * (1 << ShiftG);
    // get color components
    uint32_t b0 = color0.R();
    uint32_t g0 = color0.G();
    uint32_t r0 = color0.B();
    // fade color by amount
    b0 = b0 > FadeAmountRB ? b0 - FadeAmountRB : 0;
    g0 = g0 > FadeAmountG ? g0 - FadeAmountG : 0;
    r0 = r0 > FadeAmountRB ? r0 - FadeAmountRB : 0;
    // get color components
    uint32_t b1 = color1.R();
    uint32_t g1 = color1.G();
    uint32_t r1 = color1.B();
    // add c0 and c1 with rounding, divide by 2 and clamp to color format bit depth
    auto b = (((b0 << 1) + (b1 << 1) + 1) >> (2 + ShiftRB)) << ShiftRB;
    auto g = (((g0 << 1) + (g1 << 1) + 1) >> (2 + ShiftG)) << ShiftG;
    auto r = (((r0 << 1) + (r1 << 1) + 1) >> (2 + ShiftRB)) << ShiftRB;
    return Color::XRGB8888(r, g, b);
}

template <typename OUT_FORMAT>
auto buildFadeTable(const std::vector<Color::XRGB8888> &imgColorMap, const std::vector<Color::XRGB8888> &colorSpaceMap, uint32_t targetColorMapSize, const uint32_t fadeAmount = 1) -> std::pair<std::vector<Color::XRGB8888>, std::vector<uint8_t>>
{
    // add initial colors to queue of colors to process
    std::deque<Color::XRGB8888> colorQueue;
    std::copy(imgColorMap.cbegin(), imgColorMap.cend(), std::back_inserter(colorQueue));
    // generate all possible combinations of mixed colors
    // we generate new colors by:
    // - fade color0 rgb by fadeamount
    // - mix with color1 by adding color0 + color1 rgb and dividing by 2 with rounding
    // if we generate a colors that has not been seen yet, add it to the queue
    // loop until we have not added new colors / the queue is empty
    std::map<Color::XRGB8888, uint32_t> colorsAndCounts;
    while (!colorQueue.empty())
    {
        // pop first color from queue
        const auto color0 = colorQueue.front();
        colorQueue.pop_front();
        // check if color already in color map
        auto c0It = colorsAndCounts.find(color0);
        if (c0It == colorsAndCounts.end())
        {
            // no. add to end of colors as we need to mix it with itself too
            colorsAndCounts[color0] = 1;
        }
        // mix with other colors
        for (auto c1It = colorsAndCounts.begin(); c1It != colorsAndCounts.end(); ++c1It)
        {
            const auto color = fadeColor<OUT_FORMAT>(color0, c1It->first, fadeAmount);
            // check if color already exists in image color map
            if (std::find(imgColorMap.cbegin(), imgColorMap.cend(), color) == imgColorMap.cend())
            {
                // check if color exists in new color map
                if (auto cIt = colorsAndCounts.find(color); cIt != colorsAndCounts.end())
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
    }
    // now we have all colors that will be generated. stuff all colors into temporary "image", repeating colors if found multiple times
    std::vector<Color::XRGB8888> tempColors;
    for (auto entry : colorsAndCounts)
    {
        std::fill_n(std::back_inserter(tempColors), entry.second, entry.first);
    }
    // reduce "image" colors to targetColorMapSize colors
    ColorFit<Color::XRGB8888> colorFit(colorSpaceMap);
    auto colorMapping = colorFit.reduceColors(tempColors, targetColorMapSize - imgColorMap.size());
    REQUIRE(colorMapping.size() > 0 && colorMapping.size() <= 256 && targetColorMapSize >= colorMapping.size(), std::runtime_error, "Unexpected number of mapped colors");
    // make sure original image colors are at the start of the new color map
    std::vector<Color::XRGB8888> finalColorMap;
    std::copy(imgColorMap.cbegin(), imgColorMap.cend(), std::back_inserter(finalColorMap));
    std::transform(colorMapping.cbegin(), colorMapping.cend(), std::back_inserter(finalColorMap), [](auto entry)
                   { return entry.first; });
    // build table that maps two input colors to one output color
    std::map<uint16_t, uint8_t> fadeMap;
#pragma omp parallel for
    for (int index0 = 0; index0 < static_cast<int>(finalColorMap.size()); index0++)
    {
        const auto color0 = finalColorMap[index0];
        for (uint32_t index1 = 0; index1 < finalColorMap.size(); ++index1)
        {
            const auto newColor = fadeColor<OUT_FORMAT>(color0, finalColorMap[index1], fadeAmount);
            // find out which color is closest to the resulting color
            float bestMSE = 10.0F;
            uint8_t bestIndex = 0;
            for (uint32_t searchIndex = 0; searchIndex < finalColorMap.size(); ++searchIndex)
            {
                auto colorMSE = Color::XRGB8888::mse(newColor, finalColorMap[searchIndex]);
                if (bestMSE > colorMSE)
                {
                    bestMSE = colorMSE;
                    bestIndex = searchIndex;
                }
            }
            // store new color index
            auto colorsHash = (static_cast<uint16_t>(index0) << 8) | static_cast<uint16_t>(index1);
#pragma omp critical
            {
                fadeMap[colorsHash] = bestIndex;
            }
        }
    }
    // fill index map for the whole of the 16-bit space. empty colors map to first entry / transparent / backdrop
    std::vector<uint8_t> indexMap(65536, 0);
    for (const auto &fadeEntry : fadeMap)
    {
        indexMap[fadeEntry.first] = fadeEntry.second;
    }
    return {finalColorMap, indexMap};
}

std::vector<Image::Frame> readImages(const std::vector<std::string> &fileNames)
{
    Color::Format commonImgFormat = Color::Format::Unknown;
    Image::DataSize commonImgSize = {0, 0};
    std::vector<Image::Frame> images;
    // open first image and store type
    auto ifIt = fileNames.cbegin();
    while (ifIt != fileNames.cend())
    {
        std::cout << "Reading " << *ifIt;
        Image::Frame img;
        try
        {
            img = IO::File::readImage(*ifIt);
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
        if (ifIt == fileNames.cbegin())
        {
            // set type and size of first image
            commonImgFormat = imgFormat;
            commonImgSize = imgSize;
        }
        else
        {
            // check type and size
            REQUIRE(commonImgFormat == imgFormat, std::runtime_error, "Image color formats do not match");
            REQUIRE(commonImgSize == imgSize, std::runtime_error, "Image sizes do not match");
        }
        img.index = static_cast<uint32_t>(std::distance(fileNames.cbegin(), ifIt));
        img.fileName = *ifIt;
        images.push_back(img);
        ifIt++;
        std::cout << std::endl;
    }
    return images;
}

int main(int argc, const char *argv[])
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
        // read image(s) from disk
        auto inImages = readImages(m_inFile);
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
        // ----- convert output image(s) to paletted -----
        // combine all images into one
        std::cout << "Combining images..." << std::endl;
        const auto [inPixels, inStartIndices] = combineRawPixelData<Color::XRGB8888>(inImages, false);
        // use cluster fit to find optimum color mapping
        std::cout << "Building common color map (this might take some time)..." << std::endl;
        ColorFit<Color::XRGB8888> colorFit(colorSpaceMap);
        const auto colorMapping = colorFit.reduceColors(inPixels, m_imgcolors);
        REQUIRE(colorMapping.size() > 0 && m_imgcolors >= colorMapping.size(), std::runtime_error, "Unexpected number of mapped colors");
        // apply color map to all images
        std::cout << "Converting images..." << std::endl;
        std::vector<Image::Frame> finalImages(inImages.size());
#pragma omp parallel for
        for (int di = 0; di < static_cast<int>(inImages.size()); di++)
        {
            // convert image to paletted using dithering
            const auto &d = inImages.at(di);
            auto r = d;
            switch (m_quantizationMethod)
            {
            case Image::Quantization::Method::ClosestColor:
                r.data = Image::Quantization::quantizeClosest(d.data, colorMapping);
                break;
            case Image::Quantization::Method::AtkinsonDither:
                r.data = Image::Quantization::atkinsonDither(d.data, d.info.size.width(), d.info.size.height(), colorMapping);
                break;
            default:
                THROW(std::runtime_error, "Unsupported quantization method " << Image::Quantization::toString(m_quantizationMethod));
            }
            REQUIRE(r.data.pixels().format() == Color::Format::Paletted8, std::runtime_error, "Expected 8-bit paletted return image");
            r.info.pixelFormat = r.data.pixels().format();
            r.info.colorMapFormat = r.data.colorMap().format();
            r.info.nrOfColorMapEntries = r.data.colorMap().size();
            finalImages.at(di) = r;
        }
        // ----- create fade table -----
        std::cout << "Building fade table..." << std::endl;
        const auto quantizedColorMap = finalImages.front().data.colorMap().convertData<Color::XRGB8888>();
        const bool outFormat565 = m_outformat == Color::Format::RGB565 || m_outformat == Color::Format::BGR565;
        const auto [colorMap32, indexMap] = outFormat565 ? buildFadeTable<Color::RGB565>(quantizedColorMap, colorSpaceMap, m_outcolors, m_fadeAmount) : buildFadeTable<Color::XRGB1555>(quantizedColorMap, colorSpaceMap, m_outcolors, m_fadeAmount);
        // ----- convert color map to output format -----
        const Image::PixelData finalColorMap = Image::PixelData(colorMap32, Color::Format::XRGB8888).convertTo(m_outformat);
        auto finalImage0 = finalImages.front();
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
                    hFile << "// Converted with genfadetable " << getCommandLine(argc, argv) << std::endl;
                    hFile << "// Note that the _Alignas specifier will need C11, as a workaround use __attribute__((aligned(4)))" << std::endl
                          << std::endl;
                    // output image data info
                    hFile << "// Data is bitmap, pixel format: " << Color::formatInfo(finalImage0.data.pixels().format()).name;
                    if (finalColorMap.format() != Color::Format::Unknown)
                    {
                        hFile << ", color map format: " << Color::formatInfo(finalColorMap.format()).name;
                    }
                    hFile << std::endl
                          << std::endl;
                    // convert image data to uint32_ts
                    auto [imageData32, imageOrSpriteStartIndices] = Image::combineRawPixelData<uint32_t>(finalImages, false);
                    // make sure we have the correct number of images
                    const uint32_t nrOfImages = imageOrSpriteStartIndices.size();
                    // output image data
                    IO::Text::writeImageInfoToH(hFile, varName, imageData32, finalImage0.info.size.width(), finalImage0.info.size.height(), finalImage0.data.pixels().rawSize(), nrOfImages, false);
                    IO::Text::writeImageDataToC(cFile, varName, baseName, imageData32, imageOrSpriteStartIndices, false);
                    if (finalColorMap.format() != Color::Format::Unknown)
                    {
                        const auto paletteData8 = finalColorMap.convertDataToRaw();
                        IO::Text::writePaletteInfoToH(hFile, varName, paletteData8, finalColorMap.size(), true, false);
                        IO::Text::writePaletteDataToC(cFile, varName, paletteData8, {}, false);
                    }
                    hFile << std::endl;
                    // output fade table info
                    hFile << "// This table maps two 8-bit palette entries i0 (src pixel color) and i1 (dest pixel color) to a new palette entry io:" << std::endl;
                    hFile << "// io = fadetable[(i0 << 8) | i1]" << std::endl;
                    // output fade table data
                    const auto indexMap32 = DataHelpers::convertTo<uint32_t>(indexMap);
                    IO::Text::writeTableInfoToH(hFile, varName + "_INDEXMAP", indexMap32, indexMap.size());
                    IO::Text::writeTableDataToC(cFile, varName + "_INDEXMAP", indexMap32);
                    hFile << std::endl;
                }
                catch (const std::runtime_error &e)
                {
                    hFile.close();
                    cFile.close();
                    std::cerr << "Failed to write data to output files: " << e.what() << std::endl;
                    return 1;
                }
            }
            else
            {
                hFile.close();
                cFile.close();
                std::cerr << "Failed to open " << m_outFile << ".h, " << m_outFile << ".c for writing" << std::endl;
                return 1;
            }
        }
        std::cout << "Done" << std::endl;
    }
    catch (const std::runtime_error &e)
    {
        std::cerr << "Error: " << e.what() << std::endl;
        return 1;
    }
    return 0;
}