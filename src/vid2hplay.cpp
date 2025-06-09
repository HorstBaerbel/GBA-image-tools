#include "color/colorhelpers.h"
#include "color/conversions.h"
#include "compression/lzss.h"
#include "image/imagehelpers.h"
#include "image/imageprocessing.h"
#include "image/spritehelpers.h"
#include "io/textio.h"
#include "io/vid2hio.h"
#include "io/vid2hreader.h"
#include "processing/datahelpers.h"
#include "processing/processingoptions.h"
#include "statistics/statistics_window.h"

#include <cstdlib>
#include <cstring>
#include <filesystem>
#include <fstream>
#include <iomanip>
#include <iostream>
#include <string>
#include <thread>

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
        // set up number of cores for parallel processing
        const auto nrOfProcessors = omp_get_num_procs();
        omp_set_num_threads(nrOfProcessors);
        // fire up video reader and open video file
        Media::Vid2hReader mediaReader;
        Media::Reader::MediaInfo mediaInfo;
        try
        {
            std::cout << "Opening " << m_inFile << "..." << std::endl;
            mediaReader.open(m_inFile);
            mediaInfo = mediaReader.getInfo();
            std::cout << "Video stream #" << mediaInfo.videoStreamIndex << ": " << mediaInfo.videoCodecName << ", " << mediaInfo.videoWidth << "x" << mediaInfo.videoHeight << "@" << mediaInfo.videoFrameRateHz;
            std::cout << ", duration " << mediaInfo.videoDurationS << "s, " << mediaInfo.videoNrOfFrames << " frames" << std::endl;
            std::cout << "Audio stream #" << mediaInfo.audioStreamIndex << ": " << mediaInfo.audioCodecName << ", " << mediaInfo.audioChannels << " channels, " << mediaInfo.audioSampleRateHz << " Hz, ";
            std::cout << Audio::formatInfo(mediaInfo.audioSampleFormat).name;
            std::cout << ", duration " << mediaInfo.audioDurationS << "s, " << mediaInfo.audioNrOfSamples << " samples, offset " << mediaInfo.audioOffsetS << "s" << std::endl;
        }
        catch (const std::runtime_error &e)
        {
            std::cerr << "Failed to open video file: " << e.what() << std::endl;
            return 1;
        }
        // create statistics window
        Statistics::Window window(2 * mediaInfo.videoWidth, 2 * mediaInfo.videoHeight, "vid2hplay");
        // read video file
        const auto startTime = std::chrono::steady_clock::now();
        const double frameInterval = 1.0 / mediaInfo.videoFrameRateHz;
        for (uint32_t frameIndex = 0; frameIndex < mediaInfo.videoNrOfFrames;)
        {
            auto frame = mediaReader.readFrame();
            REQUIRE(frame.frameType != IO::FrameType::Unknown, std::runtime_error, "Bad frame type");
            // check if image frame
            if (frame.frameType == IO::FrameType::Pixels)
            {
                const auto &image = std::get<std::vector<Color::XRGB8888>>(frame.data);
                REQUIRE(image.size() == mediaInfo.videoWidth * mediaInfo.videoHeight, std::runtime_error, "Unexpected frame size");
                // update statistics
                while ((std::chrono::steady_clock::now() - startTime) < std::chrono::duration<double>(frameIndex * frameInterval))
                {
                    std::this_thread::yield();
                }
                window.displayImage(reinterpret_cast<const uint8_t *>(image.data()), image.size() * sizeof(std::remove_reference<decltype(image.front())>::type), Ui::ColorFormat::XRGB8888, mediaInfo.videoWidth, mediaInfo.videoHeight);
                ++frameIndex;
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