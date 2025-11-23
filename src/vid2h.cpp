#include "audio/audiohelpers.h"
#include "audio/audioprocessing.h"
#include "audio/resampler.h"
#include "audio/samplebuffer.h"
#include "color/colorhelpers.h"
#include "color/conversions.h"
#include "compression/lzss.h"
#include "image/imagehelpers.h"
#include "image/imageprocessing.h"
#include "image/spritehelpers.h"
#include "io/ffmpegreader.h"
#include "io/textio.h"
#include "io/vid2hio.h"
#include "io/wavwriter.h"
#include "processing/datahelpers.h"
#include "processing/processingoptions.h"
#include "statistics/statisticswindow.h"

#include <cstdlib>
#include <cstring>
#include <filesystem>
#include <fstream>
#include <iomanip>
#include <iostream>
#include <string>

#include "cxxopts/include/cxxopts.hpp"

std::string m_inFile;
std::string m_outFile;
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
        cxxopts::Options opts("vid2h", "Convert and compress a video file to .h / .c files or a binary file");
        opts.add_option("", {"h,help", "Print help"});
        opts.add_option("", {"infile", "Input video file to convert, e.g. \"foo.avi\"", cxxopts::value<std::string>()});
        opts.add_option("", {"outname", "Output file and variable name, e.g \"foo\". This will name the output files \"foo.h\" and \"foo.c\" and variable names will start with \"FOO_\"", cxxopts::value<std::string>()});
        opts.add_option("", options.audio.cxxOption);
        opts.add_option("", options.video.cxxOption);
        opts.add_option("", options.blackWhite.cxxOption);
        opts.add_option("", options.paletted.cxxOption);
        opts.add_option("", options.truecolor.cxxOption);
        opts.add_option("", options.outformat.cxxOption);
        opts.add_option("", options.addColor0.cxxOption);
        opts.add_option("", options.moveColor0.cxxOption);
        opts.add_option("", options.shiftIndices.cxxOption);
        opts.add_option("", options.pruneIndices.cxxOption);
        opts.add_option("", options.sprites.cxxOption);
        opts.add_option("", options.tiles.cxxOption);
        opts.add_option("", options.deltaImage.cxxOption);
        opts.add_option("", options.delta8.cxxOption);
        opts.add_option("", options.delta16.cxxOption);
        opts.add_option("", options.dxt.cxxOption);
        opts.add_option("", options.dxtv.cxxOption);
        // opts.add_option("", options.gvid.cxxOption);
        // opts.add_option("", options.rle.cxxOption);
        opts.add_option("", options.lz10.cxxOption);
        opts.add_option("", options.vram.cxxOption);
        opts.add_option("", options.channelFormat.cxxOption);
        opts.add_option("", options.sampleFormat.cxxOption);
        opts.add_option("", options.sampleRateHz.cxxOption);
        opts.add_option("", options.adpcm.cxxOption);
        opts.add_option("", options.printStats.cxxOption);
        opts.add_option("", options.dryRun.cxxOption);
        opts.add_option("", options.dumpImage.cxxOption);
        opts.add_option("", options.dumpAudio.cxxOption);
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
            while (pIt != positional.cend() && (m_inFile.empty() || m_outFile.empty()))
            {
                if (m_inFile.empty())
                {
                    m_inFile = *pIt++;
                }
                if (m_outFile.empty())
                {
                    m_outFile = *pIt++;
                }
            }
        }
        // check if exclusive options set
        if (!options.audio && !options.video)
        {
            std::cerr << "Can only exclude audio OR video from output" << std::endl;
            return false;
        }
        options.blackWhite.parse(result);
        options.paletted.parse(result);
        options.truecolor.parse(result);
        if ((options.blackWhite + options.paletted + options.truecolor) == 0)
        {
            std::cerr << "One format option is needed." << std::endl;
            return false;
        }
        if ((options.blackWhite + options.paletted + options.truecolor) > 1)
        {
            std::cerr << "Only a single format option is allowed." << std::endl;
            return false;
        }
        options.outformat.parse(result);
        if (!options.outformat)
        {
            std::cerr << "Output color format must be set." << std::endl;
            return false;
        }
        options.quantizationmethod.parse(result);
        options.addColor0.parse(result);
        options.moveColor0.parse(result);
        options.shiftIndices.parse(result);
        options.pruneIndices.parse(result);
        options.sprites.parse(result);
        options.dxtv.parse(result);
        options.channelFormat.parse(result);
        options.sampleFormat.parse(result);
        options.sampleRateHz.parse(result);
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
    std::cout << "Convert and compress a video file to .h / .c files or a binary file" << std::endl;
    std::cout << "Usage: vid2h IMG [IMG_CONV] [IMG_COMP] [COMP] AUD [AUD_COMP] INFILE OUTNAME" << std::endl;
    std::cout << "General options (mutually exclusive):" << std::endl;
    std::cout << options.video.helpString() << std::endl;
    std::cout << options.audio.helpString() << std::endl;
    std::cout << "Image format options (mutually exclusive):" << std::endl;
    std::cout << options.blackWhite.helpString() << std::endl;
    std::cout << options.paletted.helpString() << std::endl;
    std::cout << options.truecolor.helpString() << std::endl;
    std::cout << "Output color format (must be set):" << std::endl;
    std::cout << options.outformat.helpString() << std::endl;
    std::cout << "Image conversion options (all optional):" << std::endl;
    std::cout << options.quantizationmethod.helpString() << std::endl;
    std::cout << options.addColor0.helpString() << std::endl;
    std::cout << options.moveColor0.helpString() << std::endl;
    std::cout << options.shiftIndices.helpString() << std::endl;
    std::cout << options.pruneIndices.helpString() << std::endl;
    std::cout << options.tiles.helpString() << std::endl;
    std::cout << options.sprites.helpString() << std::endl;
    std::cout << options.deltaImage.helpString() << std::endl;
    std::cout << options.delta8.helpString() << std::endl;
    std::cout << options.delta16.helpString() << std::endl;
    std::cout << "Image compression options (mutually exclusive):" << std::endl;
    std::cout << options.dxt.helpString() << std::endl;
    std::cout << options.dxtv.helpString() << std::endl;
    // std::cout << options.gvid.helpString() << std::endl;
    std::cout << "Compression options (mutually exclusive):" << std::endl;
    // std::cout << options.rle.helpString() << std::endl;
    std::cout << options.lz10.helpString() << std::endl;
    std::cout << "Compression modifiers (optional):" << std::endl;
    std::cout << options.vram.helpString() << std::endl;
    std::cout << "Output audio format (all optional):" << std::endl;
    std::cout << options.channelFormat.helpString() << std::endl;
    std::cout << options.sampleFormat.helpString() << std::endl;
    std::cout << options.sampleRateHz.helpString() << std::endl;
    std::cout << "Audio compression options (all optional):" << std::endl;
    std::cout << options.adpcm.helpString() << std::endl;
    std::cout << "INFILE: Input video file to convert, e.g. \"foo.avi\"" << std::endl;
    std::cout << "OUTNAME: is determined from the first non-existant file path. It can be an " << std::endl;
    std::cout << "absolute or relative file path or a file base name. Two files OUTNAME.h and " << std::endl;
    std::cout << "OUTNAME.c will be generated. All variables will begin with the base name " << std::endl;
    std::cout << "portion of OUTNAME." << std::endl;
    std::cout << "Misc options (all optional):" << std::endl;
    std::cout << options.printStats.helpString() << std::endl;
    std::cout << options.dryRun.helpString() << std::endl;
    std::cout << options.dumpImage.helpString() << std::endl;
    std::cout << options.dumpAudio.helpString() << std::endl;
    std::cout << "help: Show this help." << std::endl;
    std::cout << "Image order: input, color conversion, addcolor0, movecolor0, shift, sprites, " << std::endl;
    std::cout << "tiles, deltaimage, dxtg / dtxv, delta8 / delta16, rle, lz10, output" << std::endl;
    std::cout << "Note: Multi-channel audio will be converted to stereo and sample bit depth will " << std::endl;
    std::cout << "be converted to 16 bit" << std::endl;
}

auto buildImageProcessing(const ProcessingOptions &opts) -> Image::Processing
{
    Image::Processing videoProcessing;
    if (opts.blackWhite)
    {
        videoProcessing.addStep(Image::ProcessingType::ConvertBlackWhite, {opts.quantizationmethod.value, opts.blackWhite.value});
    }
    else if (opts.paletted)
    {
        // add palette conversion using a RGB555 or RGB565 reference color map
        std::vector<Color::XRGB8888> colorSpaceMap;
        switch (opts.outformat.value)
        {
        case Color::Format::XBGR1555:
            colorSpaceMap = ColorHelpers::buildColorMapFor(Color::Format::XRGB1555);
            break;
        case Color::Format::BGR565:
            colorSpaceMap = ColorHelpers::buildColorMapFor(Color::Format::RGB565);
            break;
        default:
            colorSpaceMap = ColorHelpers::buildColorMapFor(opts.outformat.value);
        }
        videoProcessing.addStep(Image::ProcessingType::ConvertPaletted, {opts.quantizationmethod.value, opts.paletted.value, colorSpaceMap});
    }
    else if (opts.commonPalette)
    {
        // add common palette conversion using a RGB555 or RGB565 reference color map
        std::vector<Color::XRGB8888> colorSpaceMap;
        switch (opts.outformat.value)
        {
        case Color::Format::XBGR1555:
            colorSpaceMap = ColorHelpers::buildColorMapFor(Color::Format::XRGB1555);
            break;
        case Color::Format::BGR565:
            colorSpaceMap = ColorHelpers::buildColorMapFor(Color::Format::RGB565);
            break;
        default:
            colorSpaceMap = ColorHelpers::buildColorMapFor(opts.outformat.value);
        }
        videoProcessing.addStep(Image::ProcessingType::ConvertCommonPalette, {opts.quantizationmethod.value, opts.commonPalette.value, colorSpaceMap});
    }
    else if (opts.truecolor)
    {
        videoProcessing.addStep(Image::ProcessingType::ConvertTruecolor, {opts.truecolor.value});
    }
    // build image processing pipeline - conversion
    if (opts.paletted)
    {
        videoProcessing.addStep(Image::ProcessingType::ReorderColors, {});
        if (opts.addColor0)
        {
            videoProcessing.addStep(Image::ProcessingType::AddColor0, {opts.addColor0.value});
        }
        if (opts.moveColor0)
        {
            videoProcessing.addStep(Image::ProcessingType::MoveColor0, {opts.moveColor0.value});
        }
        if (opts.shiftIndices)
        {
            videoProcessing.addStep(Image::ProcessingType::ShiftIndices, {opts.shiftIndices.value});
        }
        if (opts.pruneIndices)
        {
            videoProcessing.addStep(Image::ProcessingType::PruneIndices, {});
            // TODO store 1, 2, 4 bits
            videoProcessing.addStep(Image::ProcessingType::PadColorMap, {uint32_t(16)});
        }
        else
        {
            videoProcessing.addStep(Image::ProcessingType::PadColorMap, {opts.paletted.value + (opts.addColor0 ? 1 : 0)});
        }
        videoProcessing.addStep(Image::ProcessingType::ConvertColorMapToRaw, {opts.outformat.value});
        videoProcessing.addStep(Image::ProcessingType::PadColorMapData, {uint32_t(4)});
    }
    if (opts.sprites)
    {
        videoProcessing.addStep(Image::ProcessingType::ConvertSprites, {opts.sprites.value.front()});
    }
    if (opts.tiles)
    {
        videoProcessing.addStep(Image::ProcessingType::ConvertTiles, {});
    }
    // build image processing pipeline - image compression
    if (opts.deltaImage)
    {
        videoProcessing.addStep(Image::ProcessingType::DeltaImage, {});
    }
    if (opts.dxt)
    {
        videoProcessing.addStep(Image::ProcessingType::CompressDXT, {opts.outformat.value}, true, opts.printStats);
    }
    if (opts.dxtv)
    {
        videoProcessing.addStep(Image::ProcessingType::CompressDXTV, {opts.outformat.value, opts.dxtv.value}, true, opts.printStats);
    }
    /*if (options.gvid)
    {
        processing.addStep(Image::ProcessingType::CompressGVID, {}, true, true);
    }*/
    // convert to raw data (only if not raw data already)
    videoProcessing.addStep(Image::ProcessingType::ConvertPixelsToRaw, {opts.outformat.value});
    // build image processing pipeline - entropy compression
    if (opts.delta8)
    {
        videoProcessing.addStep(Image::ProcessingType::ConvertDelta8, {});
    }
    if (opts.delta16)
    {
        videoProcessing.addStep(Image::ProcessingType::ConvertDelta16, {});
    }
    /*if (options.rle)
    {
        processing.addStep(Image::ProcessingType::CompressRLE, {options.vram.isSet}, true);
    }*/
    if (opts.lz10)
    {
        videoProcessing.addStep(Image::ProcessingType::CompressLZ10, {opts.vram.isSet}, true, opts.printStats);
    }
    videoProcessing.addStep(Image::ProcessingType::PadPixelData, {uint32_t(4)});
    return videoProcessing;
}

auto buildAudioProcessing(const ProcessingOptions &opts, const Media::Reader::MediaInfo &mediaInfo) -> Audio::Processing
{
    Audio::Processing audioProcessing;
    const auto audioOutSampleRateHz = opts.sampleRateHz ? opts.sampleRateHz.value : mediaInfo.audioSampleRateHz;
    if (opts.channelFormat || opts.sampleRateHz || opts.sampleFormat)
    {
        const auto audioOutChannelFormat = opts.channelFormat ? opts.channelFormat.value : mediaInfo.audioChannelFormat;
        const auto audioOutSampleFormat = opts.sampleFormat ? opts.sampleFormat.value : mediaInfo.audioSampleFormat;
        audioProcessing.addStep(Audio::ProcessingType::Resample, {audioOutChannelFormat, audioOutSampleRateHz, audioOutSampleFormat});
    }
    // ----- build audio processing pipeline - frame sample packaging
    // We need to provide enough samples for one frame of video at the video frame rate
    // We also need to make sure audio frames size requirements are met:
    // * Multiple of 16 int8_t samples per channel for GBA audio playback (sound DMA always copies 16 * int8_t when buffer drains)
    // * Multiple of 4 bytes per channel for NDS audio playback
    // This can result in the buffer size varying between frames, otherwise the buffer would grow during playback
    const double audioOutSamplesPerFrame = audioOutSampleRateHz / mediaInfo.videoFrameRateHz; // Number of audio samples per channel needed per frame of video
    audioProcessing.addStep(Audio::ProcessingType::Repackage, {audioOutSamplesPerFrame, uint32_t(16)});
    // build audio processing pipeline - audio compression
    if (opts.adpcm)
    {
        audioProcessing.addStep(Audio::ProcessingType::CompressADPCM, {}, true);
    }
    return audioProcessing;
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
            std::cerr << "No input file passed. Aborting." << std::endl;
            return 1;
        }
        if (m_outFile.empty())
        {
            std::cerr << "No output name passed. Aborting." << std::endl;
            return 1;
        }
        // set up number of cores for parallel processing
        const auto nrOfProcessors = omp_get_num_procs();
        omp_set_num_threads(nrOfProcessors);
        // fire up video reader and open video file
        Media::FFmpegReader mediaReader;
        Media::Reader::MediaInfo mediaInfo;
        bool sourceHasVideo = false;
        bool sourceHasAudio = false;
        try
        {
            std::cout << "Opening " << m_inFile << "..." << std::endl;
            mediaReader.open(m_inFile);
            mediaInfo = mediaReader.getInfo();
            sourceHasVideo = mediaInfo.fileType & IO::FileType::Video;
            sourceHasAudio = mediaInfo.fileType & IO::FileType::Audio;
            if (sourceHasVideo)
            {
                std::cout << "Video stream #" << mediaInfo.videoStreamIndex << ": " << mediaInfo.videoCodecName << ", " << mediaInfo.videoWidth << "x" << mediaInfo.videoHeight << "@" << mediaInfo.videoFrameRateHz;
                std::cout << ", duration " << mediaInfo.videoDurationS << "s, " << mediaInfo.videoNrOfFrames << " frames" << std::endl;
            }
            if (sourceHasAudio)
            {
                std::cout << "Audio stream #" << mediaInfo.audioStreamIndex << ": " << mediaInfo.audioCodecName << ", " << Audio::formatInfo(mediaInfo.audioChannelFormat).description << ", " << mediaInfo.audioSampleRateHz << " Hz, ";
                std::cout << Audio::formatInfo(mediaInfo.audioSampleFormat).description;
                std::cout << ", duration " << mediaInfo.audioDurationS << "s, " << mediaInfo.audioNrOfFrames << " frames, " << mediaInfo.audioNrOfSamples << " samples, offset " << mediaInfo.audioOffsetS << "s" << std::endl;
            }
        }
        catch (const std::runtime_error &e)
        {
            std::cerr << "Failed to open video file: " << e.what() << std::endl;
            return 1;
        }
        // check if processing makes sense
        if (sourceHasVideo && !options.video && !sourceHasAudio)
        {
            std::cerr << "Chose not to output video, but source has no audio. Output would be empty. Exiting..." << std::endl;
            return 1;
        }
        if (sourceHasAudio && !options.audio && !sourceHasVideo)
        {
            std::cerr << "Chose not to output audio, but source has no video. Output would be empty. Exiting..." << std::endl;
            return 1;
        }
        // ----- build image processing pipeline - input -----
        Image::Processing videoProcessing;
        if (options.video)
        {
            videoProcessing = buildImageProcessing(options);
            // print image processing pipeline configuration
            const auto imageProcessingDesc = videoProcessing.getProcessingDescription();
            std::cout << "Applying image processing: " << imageProcessingDesc << std::endl;
        }
        else
        {
            std::cout << "Ignoring video. Won't add video to output" << std::endl;
        }
        // ----- build audio processing pipeline - input -----
        Audio::Processing audioProcessing;
        if (options.audio)
        {
            audioProcessing = buildAudioProcessing(options, mediaInfo);
            // print audio processing pipeline configuration
            const auto audioProcessingDesc = audioProcessing.getProcessingDescription();
            std::cout << "Applying audio processing: " << audioProcessingDesc << std::endl;
        }
        else
        {
            std::cout << "Ignoring audio. Won't add audio to output" << std::endl;
        }
        // check if we want to write output files
        std::ofstream binFile;
        if (!options.dryRun)
        {
            // open output file and write initial file header
            binFile = std::ofstream(m_outFile + ".bin", std::ios::out | std::ios::binary);
            if (binFile.is_open())
            {
                std::cout << "Writing output file " << m_outFile << ".bin" << std::endl;
                try
                {
                    IO::Vid2h::writeDummyFileHeader(binFile);
                }
                catch (const std::runtime_error &e)
                {
                    binFile.close();
                    std::cerr << "Failed to write data to output file: " << e.what() << std::endl;
                    return 1;
                }
            }
            else
            {
                std::cerr << "Failed to open " << m_outFile << ".bin for writing" << std::endl;
                return 1;
            }
        }
        // create statistics window
        Statistics::Window window(2 * mediaInfo.videoWidth, 2 * mediaInfo.videoHeight, "vid2h");
        auto statistics = window.getStatisticsContainer();
        // process media frames
        uint32_t lastProgress = 0;
        auto startTime = std::chrono::steady_clock::now();
        // Video info
        const bool outputHasVideo = (mediaInfo.fileType & IO::FileType::Video) && options.video;
        uint32_t videoFrameIndex = 0;         // Index of last frame processed
        uint64_t videoOutCompressedSize = 0;  // Combined size of compressed video data
        uint32_t videoOutMaxMemoryNeeded = 0; // Maximum memory needed for decoding video frames
        Image::FrameInfo videoOutInfo;        // Information about video from media decoder
        // Audio info
        const bool outputHasAudio = (mediaInfo.fileType & IO::FileType::Audio) && options.audio;
        uint64_t audioOutCompressedSize = 0; // Combined size of compressed audio data
        int32_t audioFirstFrameOffset = 0;   // Offset of first audio frame in samples
        while (videoFrameIndex < mediaInfo.videoNrOfFrames && audioProcessing.nrOfInputFrames() < mediaInfo.audioNrOfFrames)
        {
            const auto inFrame = mediaReader.readFrame();
            // check if EOF
            if (inFrame.frameType == IO::FrameType::Unknown)
            {
                REQUIRE(!outputHasVideo || videoFrameIndex == (mediaInfo.videoNrOfFrames - 1), std::runtime_error, "Expected " << mediaInfo.videoNrOfFrames << " video frames, but got " << videoFrameIndex);
                REQUIRE(!outputHasAudio || audioProcessing.nrOfInputFrames() == (mediaInfo.audioNrOfFrames - 1), std::runtime_error, "Expected " << mediaInfo.audioNrOfFrames << " audio frames, but got " << audioProcessing.nrOfInputFrames());
                break;
            }
            // check if image frame
            if (inFrame.frameType == IO::FrameType::Pixels && outputHasVideo)
            {
                const auto &inImage = std::get<std::vector<Color::XRGB8888>>(inFrame.data);
                REQUIRE(inImage.size() == mediaInfo.videoWidth * mediaInfo.videoHeight, std::runtime_error, "Unexpected image size");
                // build internal image from pixels and apply processing
                const Image::FrameInfo imageInfo = {{mediaInfo.videoWidth, mediaInfo.videoHeight}, Color::Format::Unknown, Color::Format::Unknown, 0, 0};
                const Image::MapInfo mapInfo = {{0, 0}, {}};
                const auto outFrame = videoProcessing.processStream(Image::Frame{videoFrameIndex, "", Image::DataType(Image::DataType::Flags::Bitmap), imageInfo, inImage, mapInfo}, statistics);
                videoOutCompressedSize += outFrame.data.pixels().rawSize() + (options.paletted ? outFrame.data.colorMap().rawSize() : 0);
                videoOutMaxMemoryNeeded = videoOutMaxMemoryNeeded < outFrame.info.maxMemoryNeeded ? outFrame.info.maxMemoryNeeded : videoOutMaxMemoryNeeded;
                videoOutInfo = outFrame.info;
                // write image to file
                if (!options.dryRun && binFile.is_open())
                {
                    IO::Vid2h::writeFrame(binFile, outFrame);
                }
                ++videoFrameIndex;
            }
            else if (inFrame.frameType == IO::FrameType::Audio && outputHasAudio)
            {
                // build audio frame from sample data
                const Audio::Frame audioFrame = {audioProcessing.nrOfInputFrames(), "", {mediaInfo.audioSampleRateHz, mediaInfo.audioChannelFormat, mediaInfo.audioSampleFormat, false, 0}, std::get<std::vector<int16_t>>(inFrame.data), 0};
                const auto outFrameOpt = audioProcessing.processStream(audioFrame, false, statistics);
                if (outFrameOpt.has_value())
                {
                    const auto &outFrame = outFrameOpt.value();
                    audioOutCompressedSize += AudioHelpers::rawDataSize(outFrame.data);
                    // write samples to file
                    if (!options.dryRun && binFile.is_open())
                    {
                        IO::Vid2h::writeFrame(binFile, outFrame);
                    }
                }
            }
            // calculate progress
            const uint32_t newProgress = ((100 * videoFrameIndex) / mediaInfo.videoNrOfFrames);
            if (lastProgress != newProgress)
            {
                lastProgress = newProgress;
                const auto newTime = std::chrono::steady_clock::now();
                const auto timePassedMs = std::chrono::duration<double>(newTime - startTime);
                const auto fps = static_cast<double>(videoFrameIndex) / timePassedMs.count();
                const auto restS = (mediaInfo.videoNrOfFrames - videoFrameIndex) / fps;
                std::cout << std::fixed << std::setprecision(1) << lastProgress << "%, " << fps << " fps, " << restS << "s remaining" << std::endl;
            }
            // update statistics
            window.update();
        }
        // flush remaining buffers
        auto outFrameOpt = audioProcessing.processStream(Audio::Frame(), true, statistics);
        if (outFrameOpt.has_value())
        {
            const auto &outFrame = outFrameOpt.value();
            audioOutCompressedSize += AudioHelpers::rawDataSize(outFrame.data);
            // write samples to file
            if (!options.dryRun && binFile.is_open())
            {
                IO::Vid2h::writeFrame(binFile, outFrame);
            }
        }
        // write final file header to start of stream
        if (!options.dryRun && binFile.is_open())
        {
            binFile.seekp(0);
            // build headers
            IO::Vid2h::AudioHeader audioHeader;
            IO::Vid2h::VideoHeader videoHeader;
            if (outputHasAudio)
            {
                audioHeader = IO::Vid2h::createAudioHeader(audioProcessing.outputFrameInfo(), audioProcessing.nrOfOutputFrames(), audioProcessing.nrOfOutputSamples(), audioFirstFrameOffset, audioProcessing.outputMaxMemoryNeeded(), audioProcessing.getDecodingSteps());
            }
            if (outputHasVideo)
            {
                videoHeader = IO::Vid2h::createVideoHeader(videoOutInfo, mediaInfo.videoNrOfFrames, mediaInfo.videoFrameRateHz, videoOutMaxMemoryNeeded, 0, videoProcessing.getDecodingSteps());
            }
            // check which headers to add
            if (outputHasAudio && outputHasVideo)
            {
                IO::Vid2h::writeMediaFileHeader(binFile, videoHeader, audioHeader);
            }
            else if (outputHasAudio)
            {
                IO::Vid2h::writeAudioFileHeader(binFile, audioHeader);
            }
            else if (outputHasVideo)
            {
                IO::Vid2h::writeVideoFileHeader(binFile, videoHeader);
            }
            binFile.close();
        }
        // output some info about data
        if (outputHasVideo)
        {
            const auto videoInputSize = mediaInfo.videoWidth * mediaInfo.videoHeight * 3 * mediaInfo.videoNrOfFrames;
            std::cout << "Video:" << std::endl;
            std::cout << "  Video input size: " << static_cast<double>(videoInputSize) / (1024 * 1024) << " MB" << std::endl;
            std::cout << "  Compressed size: " << std::fixed << std::setprecision(2) << static_cast<double>(videoOutCompressedSize) / (1024 * 1024) << " MB" << std::endl;
            std::cout << "  Avg. bit rate: " << std::fixed << std::setprecision(2) << (static_cast<double>(videoOutCompressedSize) / 1024) / mediaInfo.videoDurationS << " kB/s" << std::endl;
            std::cout << "  Avg. frame size: " << videoOutCompressedSize / mediaInfo.videoNrOfFrames << " Byte" << std::endl;
            std::cout << "  Max. intermediate memory for decompression: " << videoOutMaxMemoryNeeded << " Byte" << std::endl;
        }
        if (outputHasAudio)
        {
            std::cout << "Audio:" << std::endl;
            std::cout << "  Compressed size: " << std::fixed << std::setprecision(2) << static_cast<double>(audioOutCompressedSize) / (1024 * 1024) << " MB" << std::endl;
            std::cout << "  Avg. frame size: " << audioOutCompressedSize / audioProcessing.nrOfOutputFrames() << " Byte" << std::endl;
            std::cout << "  Max. intermediate memory for decompression: " << audioProcessing.outputMaxMemoryNeeded() << " Byte" << std::endl;
        }
    }
    catch (const std::runtime_error &e)
    {
        std::cerr << "Error: " << e.what() << std::endl;
        return 1;
    }
    std::cout << "Done" << std::endl;
    return 0;
}