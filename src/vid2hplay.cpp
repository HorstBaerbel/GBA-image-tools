#include "color/colorhelpers.h"
#include "color/conversions.h"
#include "compression/lzss.h"
#include "image/imagehelpers.h"
#include "image/imageprocessing.h"
#include "image/spritehelpers.h"
#include "io/textio.h"
#include "io/vid2hio.h"
#include "io/vid2hreader.h"
#include "io/wavwriter.h"
#include "media/mediawindow.h"
#include "processing/datahelpers.h"
#include "processing/processingoptions.h"

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
ProcessingOptions options;

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
        // opts.add_option("", options.dumpImage.cxxOption);
        opts.add_option("", options.dumpAudio.cxxOption);
        opts.add_option("", options.dumpMeta.cxxOption);
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
    // std::cout << options.dumpImage.helpString() << std::endl;
    std::cout << options.dumpAudio.helpString() << std::endl;
    std::cout << options.dumpMeta.helpString() << std::endl;
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
        auto mediaReader = std::make_shared<Media::Vid2hReader>();
        Media::Reader::MediaInfo mediaInfo;
        bool hasVideo = false;
        bool hasAudio = false;
        bool hasMetaData = false;
        try
        {
            std::cout << "Opening " << m_inFile << "..." << std::endl;
            mediaReader->open(m_inFile);
            mediaInfo = mediaReader->getInfo();
            hasVideo = mediaInfo.fileType & IO::FileType::Video;
            hasAudio = mediaInfo.fileType & IO::FileType::Audio;
            hasMetaData = mediaInfo.metaDataSize > 0;
            if (hasVideo)
            {
                std::cout << "Video stream: " << mediaInfo.videoCodecName << ", " << mediaInfo.videoWidth << "x" << mediaInfo.videoHeight << "@" << mediaInfo.videoFrameRateHz;
                std::cout << ", duration " << mediaInfo.videoDurationS << "s, " << mediaInfo.videoNrOfFrames << " frames" << std::endl;
            }
            if (hasAudio)
            {
                std::cout << "Audio stream: " << mediaInfo.audioCodecName << ", " << Audio::formatInfo(mediaInfo.audioChannelFormat).description << ", " << mediaInfo.audioSampleRateHz << " Hz, ";
                std::cout << Audio::formatInfo(mediaInfo.audioSampleFormat).description;
                std::cout << ", duration " << mediaInfo.audioDurationS << "s, " << mediaInfo.audioNrOfFrames << " frames, " << mediaInfo.audioNrOfSamples << " samples, offset " << mediaInfo.audioOffsetS << "s" << std::endl;
            }
            if (hasMetaData)
            {
                // get first 20 bytes of meta data
                const auto metaData = mediaReader->getMetaData();
                const auto stringSize = metaData.size() > 20 ? 20 : metaData.size();
                std::string metaString(stringSize, '\0');
                std::copy(metaData.cbegin(), std::next(metaData.cbegin(), stringSize), metaString.begin());
                std::cout << "Meta data: " << mediaInfo.metaDataSize << " Bytes, first 20 bytes: \"" << metaString << "\"" << std::endl;
            }
        }
        catch (const std::runtime_error &e)
        {
            std::cerr << "Failed to open video file: " << e.what() << std::endl;
            return 1;
        }
        // get base name from input file
        const auto inFileBaseName = std::filesystem::path(m_inFile).stem();
        // check if we want to dump video or audio
        if (options.dumpAudio)
        {
            if (!hasAudio)
            {
                std::cout << "Can't dump audio. No audio in file." << std::endl;
                return 1;
            }
            try
            {
                // set up WAV writer
                IO::WavWriter wavWriter;
                const auto wavFileName = inFileBaseName.string() + "_audio.wav";
                wavWriter.open(wavFileName);
                std::cout << "Dumping audio to " << wavFileName << std::endl;
                Media::Reader::FrameData inFrame;
                do
                {
                    inFrame = mediaReader->readFrame();
                    if (inFrame.frameType == IO::FrameType::Audio)
                    {
                        const Audio::Frame audioFrame = {0, "", {mediaInfo.audioSampleRateHz, mediaInfo.audioChannelFormat, Audio::SampleFormat::Signed16P, false, 0}, std::get<std::vector<int16_t>>(inFrame.data), 0};
                        wavWriter.writeFrame(audioFrame);
                    }
                } while (inFrame.frameType != IO::FrameType::Unknown);
                wavWriter.close();
            }
            catch (const std::runtime_error &e)
            {
                std::cerr << e.what() << std::endl;
                return 1;
            }
        }
        // check if we want to dump meta data
        if (options.dumpMeta)
        {
            if (!hasMetaData)
            {
                std::cout << "Can't dump meta data. No meta data in file." << std::endl;
                return 1;
            }
            // open meta data file and dump data
            const auto metaDataFileName = inFileBaseName.string() + "_meta.bin";
            auto metaDataStream = std::ofstream(metaDataFileName, std::ios::out | std::ios::binary);
            REQUIRE(metaDataStream.is_open(), std::runtime_error, "Failed to open meta data file " << metaDataFileName << " for writing");
            std::cout << "Dumping meta data to " << metaDataFileName << std::endl;
            const auto metaData = mediaReader->getMetaData();
            metaDataStream.write(reinterpret_cast<const char *>(metaData.data()), metaData.size());
            REQUIRE(!metaDataStream.fail(), std::runtime_error, "Failed to write to meta data file");
            metaDataStream.close();
        }
        // create player window if we're not dumping
        if (!options.dumpAudio && !options.dumpMeta)
        {
            Media::Window window(2 * mediaInfo.videoWidth, 2 * mediaInfo.videoHeight, "vid2hplay");
            window.play(mediaReader);
            // wait until the player stops
            while (window.getPlayState() != Media::Window::PlayState::Stopped)
            {
                std::this_thread::sleep_for(std::chrono::milliseconds(1));
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