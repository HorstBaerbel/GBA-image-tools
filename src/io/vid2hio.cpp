#include "vid2hio.h"

#include "audio/audiohelpers.h"

namespace IO::Vid2h
{

    auto writeFileHeader(std::ostream &os, const FileType contentType) -> FileDataInfo
    {
        static_assert(sizeof(FileHeader) % 4 == 0);
        static_assert(sizeof(AudioHeader) % 4 == 0);
        static_assert(sizeof(VideoHeader) % 4 == 0);
        static_assert(sizeof(SubtitlesHeader) % 4 == 0);
        // generate file header and store it
        REQUIRE(contentType != IO::FileType::Unknown, std::runtime_error, "File content type can not be unknown");
        FileHeader fileHeader;
        fileHeader.magic = IO::Vid2h::Magic;
        fileHeader.contentType = contentType;
        os.write(reinterpret_cast<const char *>(&fileHeader), sizeof(FileHeader));
        REQUIRE(!os.fail(), std::runtime_error, "Failed to write initial file header to stream");
        // depending on the file type, make space for audio and video info
        FileDataInfo fileDataInfo;
        fileDataInfo.contentType = contentType;
        if (contentType & IO::FileType::Audio)
        {
            fileDataInfo.audioHeaderOffset = os.tellp();
            AudioHeader audioHeader;
            os.write(reinterpret_cast<const char *>(&audioHeader), sizeof(AudioHeader));
        }
        if (contentType & IO::FileType::Video)
        {
            fileDataInfo.videoHeaderOffset = os.tellp();
            VideoHeader videoHeader;
            os.write(reinterpret_cast<const char *>(&videoHeader), sizeof(VideoHeader));
        }
        if (contentType & IO::FileType::Subtitles)
        {
            fileDataInfo.subtitlesHeaderOffset = os.tellp();
            SubtitlesHeader subtitlesHeader;
            os.write(reinterpret_cast<const char *>(&subtitlesHeader), sizeof(SubtitlesHeader));
        }
        fileDataInfo.frameDataOffset = os.tellp();
        return fileDataInfo;
    }

    auto createVideoHeader(const Image::FrameInfo &imageInfo, uint32_t videoNrOfFrames, double videoFrameRateHz, uint32_t videoMemoryNeeded, uint32_t videoNrOfColorMapFrames, const std::vector<Image::ProcessingType> &decodingSteps) -> VideoHeader
    {
        REQUIRE(videoNrOfFrames < 2 ^ 16, std::runtime_error, "Number of video frames must be < 2^16");
        REQUIRE(videoMemoryNeeded < 2 ^ 24, std::runtime_error, "Max. video memory needed must be < 2^24");
        REQUIRE(videoNrOfColorMapFrames < 2 ^ 16, std::runtime_error, "Number of color map frames must be < 2^16");
        VideoHeader outHeader;
        // add video information
        const auto &pixelInfo = Color::formatInfo(imageInfo.pixelFormat);
        const auto &colorMapInfo = Color::formatInfo(imageInfo.colorMapFormat);
        outHeader.nrOfFrames = videoNrOfFrames;
        outHeader.frameRateHz = static_cast<uint32_t>(std::round(videoFrameRateHz * 65536.0));
        outHeader.width = static_cast<uint16_t>(imageInfo.size.width());
        outHeader.height = static_cast<uint16_t>(imageInfo.size.height());
        outHeader.bitsPerPixel = static_cast<uint8_t>(pixelInfo.bitsPerPixel);
        outHeader.bitsPerColor = pixelInfo.isIndexed ? static_cast<uint8_t>(colorMapInfo.bitsPerPixel) : 0;
        outHeader.colorMapEntries = pixelInfo.isIndexed ? imageInfo.nrOfColorMapEntries : 0;
        outHeader.nrOfColorMapFrames = videoNrOfColorMapFrames;
        outHeader.swappedRedBlue = (pixelInfo.isIndexed ? colorMapInfo.hasSwappedRedBlue : pixelInfo.hasSwappedRedBlue) ? 1 : 0;
        outHeader.memoryNeeded = videoMemoryNeeded;
        REQUIRE(decodingSteps.size() <= 4, std::runtime_error, "Number of decoding steps must be <= 4");
        std::memcpy(outHeader.processing, decodingSteps.data(), decodingSteps.size());
        return outHeader;
    }

    auto createAudioHeader(const Audio::FrameInfo &audioInfo, uint32_t audioNrOfFrames, uint32_t audioNrOfSamples, int32_t audioOffsetSamples, uint32_t audioMemoryNeeded, const std::vector<Audio::ProcessingType> &decodingSteps) -> AudioHeader
    {
        REQUIRE(audioNrOfFrames < 2 ^ 16, std::runtime_error, "Number of audio frames must be < 2^16");
        REQUIRE(audioMemoryNeeded < 2 ^ 16, std::runtime_error, "Max. audio memory needed must be < 2^16");
        AudioHeader outHeader;
        // add audio information
        const auto &channelInfo = Audio::formatInfo(audioInfo.channelFormat);
        const auto &sampleInfo = Audio::formatInfo(audioInfo.sampleFormat);
        outHeader.nrOfFrames = audioNrOfFrames;
        outHeader.nrOfSamples = audioNrOfSamples;
        outHeader.sampleRateHz = audioInfo.sampleRateHz;
        outHeader.channels = channelInfo.nrOfChannels;
        outHeader.sampleBits = sampleInfo.bitsPerSample;
        REQUIRE(audioOffsetSamples >= std::numeric_limits<int16_t>::min() && audioOffsetSamples <= std::numeric_limits<int16_t>::max(), std::runtime_error, "Audio offset needed must be in [" << std::numeric_limits<int16_t>::min() << "," << std::numeric_limits<int16_t>::max() << "]");
        outHeader.offsetSamples = audioOffsetSamples;
        REQUIRE(audioMemoryNeeded <= std::numeric_limits<uint16_t>::max(), std::runtime_error, "Audio memory needed must be <= " << std::numeric_limits<uint16_t>::max());
        outHeader.memoryNeeded = audioMemoryNeeded;
        REQUIRE(decodingSteps.size() <= 4, std::runtime_error, "Number of decoding steps must be <= 4");
        std::memcpy(outHeader.processing, decodingSteps.data(), decodingSteps.size());
        return outHeader;
    }

    auto createSubtitlesHeader(uint32_t subtitlesNrOfFrames) -> SubtitlesHeader
    {
        REQUIRE(subtitlesNrOfFrames < 2 ^ 16, std::runtime_error, "Number of subtitles frames must be < 2^16");
        SubtitlesHeader outHeader;
        // add subtitles information
        outHeader.nrOfFrames = subtitlesNrOfFrames;
        return outHeader;
    }

    auto writeVideoHeader(std::ostream &os, const FileDataInfo &fileDataInfo, const VideoHeader &videoHeader) -> void
    {
        REQUIRE(fileDataInfo.contentType & IO::FileType::Video, std::runtime_error, "Can't write video header to a file created without video content type");
        REQUIRE(fileDataInfo.videoHeaderOffset >= sizeof(FileHeader), std::runtime_error, "Bad video header offset");
        // seek to video header position
        os.seekp(fileDataInfo.videoHeaderOffset, std::ios_base::beg);
        REQUIRE(!os.fail(), std::runtime_error, "Failed to seek to video header position in stream");
        // store to stream
        os.write(reinterpret_cast<const char *>(&videoHeader), sizeof(VideoHeader));
        REQUIRE(!os.fail(), std::runtime_error, "Failed to write video header to stream");
    }

    auto writeAudioHeader(std::ostream &os, const FileDataInfo &fileDataInfo, const AudioHeader &audioHeader) -> void
    {
        REQUIRE(fileDataInfo.contentType & IO::FileType::Audio, std::runtime_error, "Can't write audio header to a file created without audio content type");
        REQUIRE(fileDataInfo.audioHeaderOffset >= sizeof(FileHeader), std::runtime_error, "Bad audio header offset");
        // seek to audio header position
        os.seekp(fileDataInfo.audioHeaderOffset, std::ios_base::beg);
        REQUIRE(!os.fail(), std::runtime_error, "Failed to seek to audio header position in stream");
        // store to file
        os.write(reinterpret_cast<const char *>(&audioHeader), sizeof(AudioHeader));
        REQUIRE(!os.fail(), std::runtime_error, "Failed to write audio header to stream");
    }

    auto writeSubtitlesHeader(std::ostream &os, const FileDataInfo &fileDataInfo, const SubtitlesHeader &subtitlesHeader) -> void
    {
        REQUIRE(fileDataInfo.contentType & IO::FileType::Subtitles, std::runtime_error, "Can't write subtitles header to a file created without subtitles content type");
        REQUIRE(fileDataInfo.subtitlesHeaderOffset >= sizeof(FileHeader), std::runtime_error, "Bad subtitles header offset");
        // seek to subtitles header position
        os.seekp(fileDataInfo.subtitlesHeaderOffset, std::ios_base::beg);
        REQUIRE(!os.fail(), std::runtime_error, "Failed to seek to subtitles header position in stream");
        // store to file
        os.write(reinterpret_cast<const char *>(&subtitlesHeader), sizeof(SubtitlesHeader));
        REQUIRE(!os.fail(), std::runtime_error, "Failed to write subtitles header to stream");
    }

    auto writeFrame(std::ostream &os, const Image::Frame &frame) -> void
    {
        static_assert(sizeof(FrameHeader) % 4 == 0);
        auto &imageData = frame.data;
        REQUIRE((imageData.pixels().rawSize() % 4) == 0, std::runtime_error, "Pixel data size is not a multiple of 4");
        REQUIRE((imageData.colorMap().rawSize() % 4) == 0, std::runtime_error, "Frame color map data size is not a multiple of 4");
        // convert pixel and color map data to raw format
        auto pixelData = imageData.pixels().convertDataToRaw();
        std::vector<uint8_t> colorMapData;
        if (imageData.pixels().isIndexed())
        {
            colorMapData = imageData.colorMap().convertDataToRaw();
        }
        // write color map data first
        FrameHeader frameHeader;
        if (!colorMapData.empty())
        {
            frameHeader.dataType = FrameType::Colormap;
            frameHeader.dataSize = colorMapData.size();
            os.write(reinterpret_cast<const char *>(&frameHeader), sizeof(FrameHeader));
            REQUIRE(!os.fail(), std::runtime_error, "Failed to write color map frame header for frame #" << frame.index << " to stream");
            os.write(reinterpret_cast<const char *>(colorMapData.data()), colorMapData.size());
            REQUIRE(!os.fail(), std::runtime_error, "Failed to write color map data for frame #" << frame.index << " to stream");
        }
        // write pixel data after that
        frameHeader.dataType = FrameType::Pixels;
        frameHeader.dataSize = pixelData.size();
        os.write(reinterpret_cast<const char *>(&frameHeader), sizeof(FrameHeader));
        REQUIRE(!os.fail(), std::runtime_error, "Failed to write pixel data frame header for frame #" << frame.index << " to stream");
        os.write(reinterpret_cast<const char *>(pixelData.data()), pixelData.size());
        REQUIRE(!os.fail(), std::runtime_error, "Failed to write pixel data for frame #" << frame.index << " to stream");
    }

    auto writeFrame(std::ostream &os, const Audio::Frame &frame) -> void
    {
        static_assert(sizeof(FrameHeader) % 4 == 0);
        auto sampleData = AudioHelpers::toRawData(frame.data, frame.info.channelFormat);
        REQUIRE((sampleData.size() % 4) == 0, std::runtime_error, "Audio data size is not a multiple of 4");
        // write audio data frame header
        FrameHeader frameHeader;
        frameHeader.dataType = FrameType::Audio;
        frameHeader.dataSize = sampleData.size();
        os.write(reinterpret_cast<const char *>(&frameHeader), sizeof(FrameHeader));
        REQUIRE(!os.fail(), std::runtime_error, "Failed to write audio data frame header for frame #" << frame.index << " to stream");
        // write audio sample data
        os.write(reinterpret_cast<const char *>(sampleData.data()), sampleData.size());
        REQUIRE(!os.fail(), std::runtime_error, "Failed to write audio data for frame #" << frame.index << " to stream");
    }

    auto writeFrame(std::ostream &os, const Subtitles::Frame &frame) -> void
    {
        static_assert(sizeof(FrameHeader) % 4 == 0);
        REQUIRE(frame.text.size() <= Subtitles::MaxSubTitleLength, std::runtime_error, "Max. subtitles length is " << Subtitles::MaxSubTitleLength);
        // convert subtitle to raw data (start, end, length, test)
        std::vector<uint8_t> subtitleData(4 + 4);
        subtitleData.reserve(4 + 4 + 1 + frame.text.size());
        // store times as 16:16 fixed-point
        *reinterpret_cast<int32_t *>(subtitleData.data() + 0) = static_cast<int32_t>(std::round(frame.startTimeInS * 65536.0F));
        *reinterpret_cast<int32_t *>(subtitleData.data() + 4) = static_cast<int32_t>(std::round(frame.endTimeInS * 65536.0F));
        // store string size
        subtitleData.push_back(static_cast<uint8_t>(frame.text.size()));
        // store string
        std::copy(frame.text.cbegin(), frame.text.cend(), std::back_inserter(subtitleData));
        // pad with zeros
        while (subtitleData.size() % 4 != 0)
        {
            subtitleData.push_back(0);
        }
        // write subtitles data frame header
        FrameHeader frameHeader;
        frameHeader.dataType = FrameType::Subtitles;
        frameHeader.dataSize = subtitleData.size();
        os.write(reinterpret_cast<const char *>(&frameHeader), sizeof(FrameHeader));
        REQUIRE(!os.fail(), std::runtime_error, "Failed to write subtitles data frame header for frame #" << frame.index << " to stream");
        // write subtitle data
        os.write(reinterpret_cast<const char *>(subtitleData.data()), subtitleData.size());
        REQUIRE(!os.fail(), std::runtime_error, "Failed to write subtitle data for frame #" << frame.index << " to stream");
    }

    auto writeMetaData(std::ostream &os, const FileDataInfo &fileDataInfo, const std::vector<uint8_t> &metaData) -> void
    {
        REQUIRE(!metaData.empty(), std::runtime_error, "Meta data can not be empty");
        REQUIRE(metaData.size() < 65536, std::runtime_error, "Meta data size must be < 65536 Bytes");
        // write meta data size to header
        os.seekp(0 + offsetof(FileHeader, metaDataSize), std::ios_base::beg);
        REQUIRE(!os.fail(), std::runtime_error, "Failed to seek to meta data size in stream");
        const uint16_t metaDataSize = metaData.size();
        os.write(reinterpret_cast<const char *>(&metaDataSize), sizeof(metaDataSize));
        REQUIRE(!os.fail(), std::runtime_error, "Failed to write meta data size to stream");
        // write meta data to end of file
        os.seekp(0, std::ios_base::end);
        REQUIRE(!os.fail(), std::runtime_error, "Failed to seek to end of stream");
        os.write(reinterpret_cast<const char *>(metaData.data()), metaData.size());
        REQUIRE(!os.fail(), std::runtime_error, "Failed to write meta data to end of stream");
    }

    // --------------------------------------------------------------------------------------------------------

    auto readFileHeader(std::istream &is) -> FileDataInfo
    {
        static_assert(sizeof(FileHeader) % 4 == 0);
        static_assert(sizeof(AudioHeader) % 4 == 0);
        static_assert(sizeof(VideoHeader) % 4 == 0);
        static_assert(sizeof(SubtitlesHeader) % 4 == 0);
        // seek to start of file
        is.seekg(0, std::ios_base::beg);
        REQUIRE(!is.fail(), std::runtime_error, "Failed to seek to file header position in stream");
        // read file header
        FileHeader fileHeader;
        is.read(reinterpret_cast<char *>(&fileHeader), sizeof(FileHeader));
        REQUIRE(!is.fail(), std::runtime_error, "Failed to read file header from stream");
        REQUIRE(fileHeader.magic == IO::Vid2h::Magic, std::runtime_error, "Bad file magic 0x" << std::hex << fileHeader.magic << " (expected 0x" << IO::Vid2h::Magic << ")");
        REQUIRE(fileHeader.contentType == IO::FileType::Audio || fileHeader.contentType == IO::FileType::Video || fileHeader.contentType == IO::FileType::AudioVideo, std::runtime_error, "Bad file content type");
        FileDataInfo fileDataInfo;
        fileDataInfo.contentType = fileHeader.contentType;
        if (fileHeader.contentType & IO::FileType::Audio)
        {
            fileDataInfo.audioHeaderOffset = is.tellg();
            is.seekg(sizeof(AudioHeader), std::ios_base::cur);
            REQUIRE(!is.fail(), std::runtime_error, "Failed to seek past audio header in stream");
        }
        if (fileHeader.contentType & IO::FileType::Video)
        {
            fileDataInfo.videoHeaderOffset = is.tellg();
            is.seekg(sizeof(VideoHeader), std::ios_base::cur);
            REQUIRE(!is.fail(), std::runtime_error, "Failed to seek past video header in stream");
        }
        if (fileHeader.contentType & IO::FileType::Subtitles)
        {
            fileDataInfo.subtitlesHeaderOffset = is.tellg();
            is.seekg(sizeof(SubtitlesHeader), std::ios_base::cur);
            REQUIRE(!is.fail(), std::runtime_error, "Failed to seek past subtitles header in stream");
        }
        fileDataInfo.frameDataOffset = is.tellg();
        // if stream has meta data, get its position
        if (fileHeader.metaDataSize > 0)
        {
            // seek to meta data position
            is.seekg(-fileHeader.metaDataSize, std::ios_base::end);
            REQUIRE(!is.fail(), std::runtime_error, "Failed to seek to meta data position in stream");
            fileDataInfo.metaDataOffset = is.tellg();
        }
        return fileDataInfo;
    }

    auto readVideoHeader(std::istream &is, const FileDataInfo &fileDataInfo) -> VideoHeader
    {
        REQUIRE(fileDataInfo.contentType & IO::FileType::Video, std::runtime_error, "Can't read video header from a file without video content type");
        REQUIRE(fileDataInfo.videoHeaderOffset >= sizeof(FileHeader), std::runtime_error, "Bad video header offset");
        // seek to video header position
        is.seekg(fileDataInfo.videoHeaderOffset, std::ios_base::beg);
        REQUIRE(!is.fail(), std::runtime_error, "Failed to seek to video header position in stream");
        // read header
        VideoHeader videoHeader;
        is.read(reinterpret_cast<char *>(&videoHeader), sizeof(VideoHeader));
        REQUIRE(!is.fail(), std::runtime_error, "Failed to read video header from stream");
        return videoHeader;
    }

    auto readAudioHeader(std::istream &is, const FileDataInfo &fileDataInfo) -> AudioHeader
    {
        REQUIRE(fileDataInfo.contentType & IO::FileType::Audio, std::runtime_error, "Can't read audio header from a file without audio content type");
        REQUIRE(fileDataInfo.audioHeaderOffset >= sizeof(FileHeader), std::runtime_error, "Bad audio header offset");
        // seek to audio header position
        is.seekg(fileDataInfo.audioHeaderOffset, std::ios_base::beg);
        REQUIRE(!is.fail(), std::runtime_error, "Failed to seek to audio header position in stream");
        // read header
        AudioHeader audioHeader;
        is.read(reinterpret_cast<char *>(&audioHeader), sizeof(AudioHeader));
        REQUIRE(!is.fail(), std::runtime_error, "Failed to read audio header from stream");
        return audioHeader;
    }

    auto readSubtitlesHeader(std::istream &is, const FileDataInfo &fileDataInfo) -> SubtitlesHeader
    {
        REQUIRE(fileDataInfo.contentType & IO::FileType::Subtitles, std::runtime_error, "Can't read subtitles header from a file without subtitles content type");
        REQUIRE(fileDataInfo.subtitlesHeaderOffset >= sizeof(FileHeader), std::runtime_error, "Bad subtitles header offset");
        // seek to subtitles header position
        is.seekg(fileDataInfo.subtitlesHeaderOffset, std::ios_base::beg);
        REQUIRE(!is.fail(), std::runtime_error, "Failed to seek to subtitles header position in stream");
        // read header
        SubtitlesHeader subtitlesHeader;
        is.read(reinterpret_cast<char *>(&subtitlesHeader), sizeof(SubtitlesHeader));
        REQUIRE(!is.fail(), std::runtime_error, "Failed to read subtitles header from stream");
        return subtitlesHeader;
    }

    auto readFrame(std::istream &is) -> std::pair<IO::FrameType, std::vector<uint8_t>>
    {
        // check if we're at the end of the file
        if (is.eof() || is.peek() == std::istream::traits_type::eof())
        {
            return {IO::FrameType::Unknown, {}};
        }
        // read frame header
        static_assert(sizeof(FrameHeader) % 4 == 0);
        FrameHeader frameHeader;
        is.read(reinterpret_cast<char *>(&frameHeader), sizeof(FrameHeader));
        REQUIRE(!is.fail(), std::runtime_error, "Failed to read frame header from stream");
        // allocate memory
        std::vector<uint8_t> frameData;
        frameData.resize(frameHeader.dataSize);
        // read data
        is.read(reinterpret_cast<char *>(frameData.data()), frameData.size());
        if (is.fail())
        {
            switch (frameHeader.dataType)
            {
            case FrameType::Pixels:
                THROW(std::runtime_error, "Failed to read pixel data from stream");
            case FrameType::Colormap:
                THROW(std::runtime_error, "Failed to read color map data from stream");
            case FrameType::Audio:
                THROW(std::runtime_error, "Failed to read audio data from stream");
            case FrameType::Subtitles:
                THROW(std::runtime_error, "Failed to read subtitles data from stream");
            default:
                THROW(std::runtime_error, "Got bad data type " << static_cast<uint32_t>(frameHeader.dataType) << " from stream");
            }
        }
        return {IO::FrameType(frameHeader.dataType), frameData};
    }

    auto readMetaData(std::istream &is, const FileDataInfo &fileDataInfo) -> std::vector<uint8_t>
    {
        REQUIRE(fileDataInfo.metaDataOffset >= sizeof(FileHeader), std::runtime_error, "Bad meta data offset");
        // read meta data size from header
        is.seekg(0 + offsetof(FileHeader, metaDataSize), std::ios_base::beg);
        REQUIRE(!is.fail(), std::runtime_error, "Failed to seek to meta data size in stream");
        uint16_t metaDataSize = 0;
        is.read(reinterpret_cast<char *>(&metaDataSize), sizeof(metaDataSize));
        REQUIRE(!is.fail(), std::runtime_error, "Failed to read meta data size from stream");
        if (metaDataSize == 0)
        {
            return std::vector<uint8_t>();
        }
        // seek to meta data position
        is.seekg(fileDataInfo.metaDataOffset, std::ios_base::beg);
        REQUIRE(!is.fail(), std::runtime_error, "Failed to seek to meta data position in stream");
        // read meta data
        std::vector<uint8_t> metaData(metaDataSize, 0);
        is.read(reinterpret_cast<char *>(metaData.data()), metaData.size());
        REQUIRE(!is.fail(), std::runtime_error, "Failed to read meta data from stream");
        return metaData;
    }
}
