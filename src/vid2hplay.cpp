#include "color/colorhelpers.h"
#include "color/conversions.h"
#include "compression/lzss.h"
#include "io/binreader.h"
#include "io/streamio.h"
#include "io/textio.h"
#include "processing/datahelpers.h"
#include "processing/imagehelpers.h"
#include "processing/imageprocessing.h"
#include "processing/processingoptions.h"
#include "processing/spritehelpers.h"
#include "statistics/statistics_window.h"

#include <cstdlib>
#include <cstring>
#include <filesystem>
#include <fstream>
#include <iomanip>
#include <iostream>
#include <string>

#include "cxxopts/include/cxxopts.hpp"

std::string m_inFile;

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
        cxxopts::Options opts("vid2hplay", "Play vid2h video file");
        opts.add_option("", {"h,help", "Print help"});
        opts.add_option("", {"infile", "Input video file, e.g. \"foo.bin\"", cxxopts::value<std::string>()});
        opts.parse_positional({"infile"});
        auto result = opts.parse(argc, argv);
        // check if help was requested
        if (result.count("h"))
        {
            return false;
        }
        // get input file
        if (result.count("infile"))
        {
            m_inFile = result["infile"].as<std::string>();
            if (!std::filesystem::exists(m_inFile))
            {
                std::cout << "Input file \"" << m_inFile << "\" does not exist!" << std::endl;
                return false;
            }
        }
        // if input or output are empty, get positional arguments as input first, then output
        if (result.count("positional"))
        {
            auto positional = result["positional"].as<std::vector<std::string>>();
            auto pIt = positional.cbegin();
            while (pIt != positional.cend() && m_inFile.empty())
            {
                if (m_inFile.empty())
                {
                    m_inFile = *pIt++;
                }
            }
        }
    }
    catch (const cxxopts::exceptions::parsing &e)
    {
        std::cerr << "Argument error: " << e.what() << std::endl;
        return false;
    }
    return true;
}

void printUsage()
{
    // 80 chars:  --------------------------------------------------------------------------------
    std::cout << "Play vid2h video file" << std::endl;
    std::cout << "Usage: vid2hplay INFILE" << std::endl;
}

int main(int argc, const char *argv[])
{
    try
    {
        // check arguments
        if (argc < 2 || !readArguments(argc, argv))
        {
            printUsage();
            return 2;
        }
        // check input and output
        if (m_inFile.empty())
        {
            std::cerr << "No input file passed. Aborting." << std::endl;
            return 1;
        }
        // fire up video reader and open video file
        Video::BinReader videoReader;
        Video::Reader::VideoInfo videoInfo;
        try
        {
            std::cout << "Opening " << m_inFile << "..." << std::endl;
            videoReader.open(m_inFile);
            videoInfo = videoReader.getInfo();
            std::cout << "Video stream #" << videoInfo.videoStreamIndex << ": " << videoInfo.codecName << ", " << videoInfo.width << "x" << videoInfo.height << "@" << videoInfo.fps;
            std::cout << ", duration " << videoInfo.durationS << "s, " << videoInfo.nrOfFrames << " frames" << std::endl;
        }
        catch (const std::runtime_error &e)
        {
            std::cerr << "Failed to open video file: " << e.what() << std::endl;
            return 1;
        }
        // create statistics window
        Statistics::Window window(2 * videoInfo.width, 2 * videoInfo.height);
        // read video file
        uint32_t frameIndex = 0;
        std::vector<Image::Data> images;
        do
        {
            auto frame = videoReader.readFrame();
            if (frame.empty())
            {
                break;
            }
            REQUIRE(frame.size() == videoInfo.width * videoInfo.height, std::runtime_error, "Unexpected frame size");
            // ...
            // update statistics
            window.update();
        } while (true);
    }
    catch (const std::runtime_error &e)
    {
        std::cerr << "Error: " << e.what() << std::endl;
        return 1;
    }
    return 0;
}