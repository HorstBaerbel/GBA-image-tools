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
    std::cout << "Usage: vid2h IMG [IMG_CONV] [IMG_COMP] [COMP] AUD INFILE OUTNAME" << std::endl;
    std::cout << "IMaGe format options (mutually exclusive):" << std::endl;
    std::cout << options.blackWhite.helpString() << std::endl;
    std::cout << options.paletted.helpString() << std::endl;
    std::cout << options.truecolor.helpString() << std::endl;
    std::cout << "Output color format (must be set):" << std::endl;
    std::cout << options.outformat.helpString() << std::endl;
    std::cout << "IMaGe CONVert options (all optional):" << std::endl;
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
    std::cout << "IMaGe COMPress options (mutually exclusive):" << std::endl;
    std::cout << options.dxt.helpString() << std::endl;
    std::cout << options.dxtv.helpString() << std::endl;
    // std::cout << options.gvid.helpString() << std::endl;
    std::cout << "COMPress options (mutually exclusive):" << std::endl;
    // std::cout << options.rle.helpString() << std::endl;
    std::cout << options.lz10.helpString() << std::endl;
    std::cout << "COMPress modifiers (optional):" << std::endl;
    std::cout << options.vram.helpString() << std::endl;
    std::cout << "AUDio options:" << std::endl;
    std::cout << options.channelFormat.helpString() << std::endl;
    std::cout << options.sampleFormat.helpString() << std::endl;
    std::cout << options.sampleRateHz.helpString() << std::endl;
    std::cout << "INFILE: Input video file to convert, e.g. \"foo.avi\"" << std::endl;
    std::cout << "OUTNAME: is determined from the first non-existant file path. It can be an " << std::endl;
    std::cout << "absolute or relative file path or a file base name. Two files OUTNAME.h and " << std::endl;
    std::cout << "OUTNAME.c will be generated. All variables will begin with the base name " << std::endl;
    std::cout << "portion of OUTNAME." << std::endl;
    std::cout << "MISC options (all optional):" << std::endl;
    std::cout << options.dryRun.helpString() << std::endl;
    std::cout << options.dumpImage.helpString() << std::endl;
    std::cout << options.dumpAudio.helpString() << std::endl;
    std::cout << "help: Show this help." << std::endl;
    std::cout << "Image order: input, color conversion, addcolor0, movecolor0, shift, sprites, " << std::endl;
    std::cout << "tiles, deltaimage, dxtg / dtxv, delta8 / delta16, rle, lz10, output" << std::endl;
    std::cout << "Note: Multi-channel audio will be converted to stereo and sample bit depth will " << std::endl;
    std::cout << "be converted to 16 bit" << std::endl;
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
        try
        {
            std::cout << "Opening " << m_inFile << "..." << std::endl;
            mediaReader.open(m_inFile);
            mediaInfo = mediaReader.getInfo();
            std::cout << "Video stream #" << mediaInfo.videoStreamIndex << ": " << mediaInfo.videoCodecName << ", " << mediaInfo.videoWidth << "x" << mediaInfo.videoHeight << "@" << mediaInfo.videoFrameRateHz;
            std::cout << ", duration " << mediaInfo.videoDurationS << "s, " << mediaInfo.videoNrOfFrames << " frames" << std::endl;
            std::cout << "Audio stream #" << mediaInfo.audioStreamIndex << ": " << mediaInfo.audioCodecName << ", " << Audio::formatInfo(mediaInfo.audioChannelFormat).description << ", " << mediaInfo.audioSampleRateHz << " Hz, ";
            std::cout << Audio::formatInfo(mediaInfo.audioSampleFormat).description;
            std::cout << ", duration " << mediaInfo.audioDurationS << "s, " << mediaInfo.audioNrOfSamples << " samples, offset " << mediaInfo.audioOffsetS << "s" << std::endl;
        }
        catch (const std::runtime_error &e)
        {
            std::cerr << "Failed to open video file: " << e.what() << std::endl;
            return 1;
        }
        // build processing pipeline - input
        Image::Processing imageProcessing;
        if (options.blackWhite)
        {
            imageProcessing.addStep(Image::ProcessingType::ConvertBlackWhite, {options.quantizationmethod.value, options.blackWhite.value});
        }
        else if (options.paletted)
        {
            // add palette conversion using a RGB555 or RGB565 reference color map
            std::vector<Color::XRGB8888> colorSpaceMap;
            switch (options.outformat.value)
            {
            case Color::Format::XBGR1555:
                colorSpaceMap = ColorHelpers::buildColorMapFor(Color::Format::XRGB1555);
                break;
            case Color::Format::BGR565:
                colorSpaceMap = ColorHelpers::buildColorMapFor(Color::Format::RGB565);
                break;
            default:
                colorSpaceMap = ColorHelpers::buildColorMapFor(options.outformat.value);
            }
            imageProcessing.addStep(Image::ProcessingType::ConvertPaletted, {options.quantizationmethod.value, options.paletted.value, colorSpaceMap});
        }
        else if (options.commonPalette)
        {
            // add common palette conversion using a RGB555 or RGB565 reference color map
            std::vector<Color::XRGB8888> colorSpaceMap;
            switch (options.outformat.value)
            {
            case Color::Format::XBGR1555:
                colorSpaceMap = ColorHelpers::buildColorMapFor(Color::Format::XRGB1555);
                break;
            case Color::Format::BGR565:
                colorSpaceMap = ColorHelpers::buildColorMapFor(Color::Format::RGB565);
                break;
            default:
                colorSpaceMap = ColorHelpers::buildColorMapFor(options.outformat.value);
            }
            imageProcessing.addStep(Image::ProcessingType::ConvertCommonPalette, {options.quantizationmethod.value, options.commonPalette.value, colorSpaceMap});
        }
        else if (options.truecolor)
        {
            imageProcessing.addStep(Image::ProcessingType::ConvertTruecolor, {options.truecolor.value});
        }
        // build processing pipeline - conversion
        if (options.paletted)
        {
            imageProcessing.addStep(Image::ProcessingType::ReorderColors, {});
            if (options.addColor0)
            {
                imageProcessing.addStep(Image::ProcessingType::AddColor0, {options.addColor0.value});
            }
            if (options.moveColor0)
            {
                imageProcessing.addStep(Image::ProcessingType::MoveColor0, {options.moveColor0.value});
            }
            if (options.shiftIndices)
            {
                imageProcessing.addStep(Image::ProcessingType::ShiftIndices, {options.shiftIndices.value});
            }
            if (options.pruneIndices)
            {
                imageProcessing.addStep(Image::ProcessingType::PruneIndices, {});
                // TODO store 1, 2, 4 bits
                imageProcessing.addStep(Image::ProcessingType::PadColorMap, {uint32_t(16)});
            }
            else
            {
                imageProcessing.addStep(Image::ProcessingType::PadColorMap, {options.paletted.value + (options.addColor0 ? 1 : 0)});
            }
            imageProcessing.addStep(Image::ProcessingType::ConvertColorMapToRaw, {options.outformat.value});
            imageProcessing.addStep(Image::ProcessingType::PadColorMapData, {uint32_t(4)});
        }
        if (options.sprites)
        {
            imageProcessing.addStep(Image::ProcessingType::ConvertSprites, {options.sprites.value.front()});
        }
        if (options.tiles)
        {
            imageProcessing.addStep(Image::ProcessingType::ConvertTiles, {});
        }
        // image compression
        if (options.deltaImage)
        {
            imageProcessing.addStep(Image::ProcessingType::DeltaImage, {});
        }
        if (options.dxt)
        {
            imageProcessing.addStep(Image::ProcessingType::CompressDXT, {options.outformat.value}, true, true);
        }
        if (options.dxtv)
        {
            imageProcessing.addStep(Image::ProcessingType::CompressDXTV, {options.outformat.value, options.dxtv.value}, true, true);
        }
        /*if (options.gvid)
        {
            processing.addStep(Image::ProcessingType::CompressGVID, {}, true, true);
        }*/
        // convert to raw data (only if not raw data already)
        imageProcessing.addStep(Image::ProcessingType::ConvertPixelsToRaw, {options.outformat.value});
        // entropy compression
        if (options.delta8)
        {
            imageProcessing.addStep(Image::ProcessingType::ConvertDelta8, {});
        }
        if (options.delta16)
        {
            imageProcessing.addStep(Image::ProcessingType::ConvertDelta16, {});
        }
        /*if (options.rle)
        {
            processing.addStep(Image::ProcessingType::CompressRLE, {options.vram.isSet}, true);
        }*/
        if (options.lz10)
        {
            imageProcessing.addStep(Image::ProcessingType::CompressLZ10, {options.vram.isSet}, true);
        }
        imageProcessing.addStep(Image::ProcessingType::PadPixelData, {uint32_t(4)});
        // audio processing
        auto audioOutChannelFormat = options.channelFormat ? options.channelFormat.value : mediaInfo.audioChannelFormat;
        auto audioOutSampleRateHz = options.sampleRateHz ? options.sampleRateHz.value : mediaInfo.audioSampleRateHz;
        auto audioOutSampleFormat = options.sampleFormat ? options.sampleFormat.value : mediaInfo.audioSampleFormat;
        std::shared_ptr<Audio::Resampler> resampler;
        if (options.channelFormat || options.sampleRateHz || options.sampleFormat)
        {
            resampler = std::make_shared<Audio::Resampler>(mediaInfo.audioChannelFormat, mediaInfo.audioSampleRateHz, audioOutChannelFormat, audioOutSampleRateHz, audioOutSampleFormat);
            std::cout << "Converting audio to: " << Audio::formatInfo(audioOutChannelFormat).description << ", " << audioOutSampleRateHz << " Hz, " << Audio::formatInfo(audioOutSampleFormat).description << std::endl;
        }
        // audio dumping
        std::shared_ptr<IO::WavWriter> wavWriter;
        if (options.dumpAudio)
        {
            wavWriter = std::make_shared<IO::WavWriter>();
            try
            {
                wavWriter->open("result.wav");
                std::cout << "Dumping result audio to result.wav" << std::endl;
            }
            catch (const std::runtime_error &e)
            {
                std::cerr << e.what() << std::endl;
                return 1;
            }
        }

        // print image processing pipeline configuration
        const auto processingDescription = imageProcessing.getProcessingDescription();
        std::cout << "Applying image processing: " << processingDescription << std::endl;
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
        uint32_t videoFrameIndex = 0;      // Index of last frame processed
        uint64_t videoOutCompressedSize = 0;  // Combined size of compressed video data
        uint32_t videoOutMaxMemoryNeeded = 0; // Maximum memory needed for decoding video frames
        Image::FrameInfo videoOutInfo;        // Information about video from media decoder
        // Audio info
        uint32_t audioFrameIndex = 0;                                                              // Index of last audio frame processed
        uint64_t audioOutCompressedSize = 0;                                                       // Combined size of compressed audio data
        uint32_t audioOutMaxMemoryNeeded = 0;                                                      // Maximum memory needed for decoding audio frames
        int32_t audioFirstFrameOffset = 0;                                                         // Offset of first audio frame in samples
        const double audioSamplesPerFrame = audioOutSampleRateHz / mediaInfo.videoFrameRateHz;     // Number of audio samples per channel needed per frame of video
        double audioSampleDeltaPrevFrame = 0;                                                      // Number of audio samples per channel last frame in comparison to audioSamplesPerFrame
        Audio::SampleBuffer audioSampleBuffer(audioOutChannelFormat, audioOutSampleRateHz, audioOutSampleFormat); // Buffer for audio samples from media decoder
        Audio::FrameInfo audioOutInfo;
        uint32_t audioOutNrOfAudioFrames = 0;
        uint32_t audioOutNrOfAudioSamples = 0;
        while (videoFrameIndex < mediaInfo.videoNrOfFrames && audioFrameIndex < mediaInfo.audioNrOfFrames)
        {
            const auto inFrame = mediaReader.readFrame();
            // check if EOF
            if (inFrame.frameType == IO::FrameType::Unknown)
            {
                REQUIRE(videoFrameIndex == mediaInfo.videoNrOfFrames, std::runtime_error, "Expected " << mediaInfo.videoNrOfFrames << " video frames, but got " << videoFrameIndex);
                REQUIRE(audioFrameIndex == mediaInfo.audioNrOfFrames, std::runtime_error, "Expected " << mediaInfo.audioNrOfFrames << " audio frames, but got " << audioFrameIndex);
                break;
            }
            // check if image frame
            if (inFrame.frameType == IO::FrameType::Pixels)
            {
                const auto &inImage = std::get<std::vector<Color::XRGB8888>>(inFrame.data);
                REQUIRE(inImage.size() == mediaInfo.videoWidth * mediaInfo.videoHeight, std::runtime_error, "Unexpected image size");
                // build internal image from pixels and apply processing
                Image::FrameInfo imageInfo = {{mediaInfo.videoWidth, mediaInfo.videoHeight}, Color::Format::Unknown, Color::Format::Unknown, 0, 0};
                Image::MapInfo mapInfo = {{0, 0}, {}};
                auto outFrame = imageProcessing.processStream(Image::Frame{videoFrameIndex, "", Image::DataType(Image::DataType::Flags::Bitmap), imageInfo, inImage, mapInfo}, statistics);
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
            else if (inFrame.frameType == IO::FrameType::Audio)
            {
                // build audio frame from sample data
                Audio::Frame audioFrame = {audioFrameIndex, "", {mediaInfo.audioSampleRateHz, mediaInfo.audioChannelFormat, mediaInfo.audioSampleFormat, false, 0}, std::get<std::vector<int16_t>>(inFrame.data)};
                // pipe samples through resampler to adjust channels, sample rate and sample format
                // on the last audio frame from the media file, flush the resampling buffer
                const bool isLastAudioFrame = audioFrameIndex == mediaInfo.audioNrOfFrames - 1;
                audioFrame = resampler ? resampler->resample(audioFrame, isLastAudioFrame) : audioFrame;
                // append output samples to sample buffer
                audioSampleBuffer.push_back(audioFrame);
                // dump the audio frame if user wants to
                if (wavWriter)
                {
                    wavWriter->writeFrame(audioFrame);
                }
                // We need to provide enough samples for one frame of video at the video frame rate
                // We also need to make sure audio frames size requirements are met:
                // * Multiple of 16 int8_t samples per channel for GBA audio playback
                // * Multiple of 4 bytes per channel for NDS audio playback
                // This can result in the buffer size varying between frames, otherwise the buffer would grow during playback
                auto audioSamplesThisFrame = static_cast<int32_t>(std::ceil(audioSamplesPerFrame - audioSampleDeltaPrevFrame));
                // round up to multiple of 16 samples
                if (audioSamplesThisFrame % 16 != 0)
                {
                    audioSamplesThisFrame += 16 - (audioSamplesThisFrame % 16);
                }
                // check if our buffer has this many samples
                if (audioSampleBuffer.nrOfSamplesPerChannel() >= audioSamplesThisFrame)
                {
                    auto outFrame = audioSampleBuffer.pop_front(audioSamplesThisFrame);
                    outFrame.index = audioOutNrOfAudioFrames;
                    audioOutInfo = outFrame.info;
                    // get sample information
                    const auto sampleDataSize = AudioHelpers::rawDataSize(outFrame.data);
                    audioOutCompressedSize += sampleDataSize;
                    audioOutMaxMemoryNeeded = audioOutMaxMemoryNeeded < sampleDataSize ? sampleDataSize : audioOutMaxMemoryNeeded;
                    audioSampleDeltaPrevFrame += audioSamplesThisFrame - audioSamplesPerFrame;
                    audioOutNrOfAudioSamples += audioSamplesThisFrame;
                    // add processing info to sample data
                    outFrame = Audio::Processing::prependProcessingInfo(outFrame, sampleDataSize, Audio::ProcessingType::Uncompressed, true);
                    // write samples to file
                    if (!options.dryRun && binFile.is_open())
                    {
                        IO::Vid2h::writeFrame(binFile, outFrame);
                    }
                    ++audioOutNrOfAudioFrames;
                }
                ++audioFrameIndex;
            }
            // calculate progress
            uint32_t newProgress = ((100 * videoFrameIndex) / mediaInfo.videoNrOfFrames);
            if (lastProgress != newProgress)
            {
                lastProgress = newProgress;
                auto newTime = std::chrono::steady_clock::now();
                auto timePassedMs = std::chrono::duration<double>(newTime - startTime);
                auto fps = static_cast<double>(videoFrameIndex) / timePassedMs.count();
                auto restS = (mediaInfo.videoNrOfFrames - videoFrameIndex) / fps;
                std::cout << std::fixed << std::setprecision(1) << lastProgress << "%, " << fps << " fps, " << restS << "s remaining" << std::endl;
            }
            // update statistics
            window.update();
        }
        // check if we have audio samples left
        const auto audioSamplesRemaining = audioSampleBuffer.nrOfSamplesPerChannel();
        if (audioSamplesRemaining > 0)
        {
            Audio::Frame outFrame = audioSampleBuffer.pop_front(audioSamplesRemaining);
            outFrame.index = audioOutNrOfAudioFrames;
            audioOutInfo = outFrame.info;
            // get sample information
            const auto sampleDataSize = AudioHelpers::rawDataSize(outFrame.data);
            audioOutCompressedSize += sampleDataSize;
            audioOutMaxMemoryNeeded = audioOutMaxMemoryNeeded < sampleDataSize ? sampleDataSize : audioOutMaxMemoryNeeded;
            audioOutNrOfAudioSamples += audioSamplesRemaining;
            // add processing info to sample data
            outFrame = Audio::Processing::prependProcessingInfo(outFrame, sampleDataSize, Audio::ProcessingType::Uncompressed, true);
            // write samples to file
            if (!options.dryRun && binFile.is_open())
            {
                IO::Vid2h::writeFrame(binFile, outFrame);
            }
            ++audioOutNrOfAudioFrames;
        }
        // write final file header to start of stream
        if (!options.dryRun && binFile.is_open())
        {
            binFile.seekp(0);
            IO::Vid2h::writeMediaFileHeader(binFile, videoOutInfo, mediaInfo.videoNrOfFrames, mediaInfo.videoFrameRateHz, videoOutMaxMemoryNeeded, 0, audioOutInfo, audioOutNrOfAudioFrames, audioOutNrOfAudioSamples, 0, audioOutMaxMemoryNeeded);
            binFile.close();
        }
        // close wave writer if open
        if (wavWriter)
        {
            wavWriter->close();
        }
        // output some info about data
        const auto videoInputSize = mediaInfo.videoWidth * mediaInfo.videoHeight * 3 * mediaInfo.videoNrOfFrames;
        std::cout << "Video:" << std::endl;
        std::cout << "  Video input size: " << static_cast<double>(videoInputSize) / (1024 * 1024) << " MB" << std::endl;
        std::cout << "  Compressed size: " << std::fixed << std::setprecision(2) << static_cast<double>(videoOutCompressedSize) / (1024 * 1024) << " MB" << std::endl;
        std::cout << "  Avg. bit rate: " << std::fixed << std::setprecision(2) << (static_cast<double>(videoOutCompressedSize) / 1024) / mediaInfo.videoDurationS << " kB/s" << std::endl;
        std::cout << "  Avg. frame size: " << videoOutCompressedSize / mediaInfo.videoNrOfFrames << " Byte" << std::endl;
        std::cout << "  Max. intermediate memory for decompression: " << videoOutMaxMemoryNeeded << " Byte" << std::endl;
        std::cout << "Audio:" << std::endl;
        std::cout << "  Compressed size: " << std::fixed << std::setprecision(2) << static_cast<double>(audioOutCompressedSize) / (1024 * 1024) << " MB" << std::endl;
        std::cout << "  Max. intermediate memory for decompression: " << audioOutMaxMemoryNeeded << " Byte" << std::endl;
    }
    catch (const std::runtime_error &e)
    {
        std::cerr << "Error: " << e.what() << std::endl;
        return 1;
    }
    std::cout << "Done" << std::endl;
    return 0;
}