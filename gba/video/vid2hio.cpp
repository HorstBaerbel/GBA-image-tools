#include "vid2hio.h"

#include "memory/memory.h"

namespace IO::Vid2h
{

    auto GetInfo(const uint32_t *data, const uint32_t size) -> Info
    {
        static_assert(sizeof(AudioHeader) % 4 == 0);
        static_assert(sizeof(VideoHeader) % 4 == 0);
        static_assert(sizeof(FileHeader) % 4 == 0);
        static_assert(sizeof(SubtitlesHeader) % 4 == 0);
        Info info;
        Memory::memcpy32(&info, data, sizeof(FileHeader) / 4);
        info.data = data;
        // locate meta data in file
        info.metaData = info.metaDataSize > 0 ? reinterpret_cast<const uint8_t *>(data) + size - info.metaDataSize : nullptr;
        // depending on content type, read audio and video header
        data += sizeof(FileHeader) / 4;
        if (info.contentType & IO::FileType::Audio)
        {
            Memory::memcpy32(&info.audio, data, sizeof(AudioHeader) / 4);
            data += sizeof(AudioHeader) / 4;
            // count processing stages
            info.nrOfAudioProcessings = 0;
            while (info.audio.processing[info.nrOfAudioProcessings] != Audio::ProcessingType::Invalid)
            {
                ++info.nrOfAudioProcessings;
            }
        }
        if (info.contentType & IO::FileType::Video)
        {
            Memory::memcpy32(&info.video, data, sizeof(VideoHeader) / 4);
            data += sizeof(VideoHeader) / 4;
            info.imageSize = info.video.width * info.video.height;
            switch (info.video.bitsPerColor)
            {
            case 1:
                info.imageSize = (info.imageSize + 7) / 8;
                break;
            case 2:
                info.imageSize = (info.imageSize + 3) / 4;
                break;
            case 4:
                info.imageSize = (info.imageSize + 1) / 2;
                break;
            case 8:
                info.imageSize *= 1;
                break;
            case 15:
            case 16:
                info.imageSize *= 2;
                break;
            case 24:
                info.imageSize *= 3;
                break;
            default:
                // TODO: What?
                break;
            }
            info.colorMapSize = info.video.colorMapEntries;
            switch (info.video.bitsPerColor)
            {
            case 15:
            case 16:
                info.colorMapSize *= 2;
                break;
            case 24:
                info.colorMapSize *= 3;
                break;
            default:
                // TODO: What?
                break;
            }
            // count processing stages
            info.nrOfVideoProcessings = 0;
            while (info.video.processing[info.nrOfVideoProcessings] != Image::ProcessingType::Invalid)
            {
                ++info.nrOfVideoProcessings;
            }
        }
        if (info.contentType & IO::FileType::Subtitles)
        {
            Memory::memcpy32(&info.subtitles, data, sizeof(SubtitlesHeader) / 4);
            data += sizeof(SubtitlesHeader) / 4;
        }
        info.nrOfFrames = info.video.nrOfFrames + info.video.nrOfColorMapFrames + info.audio.nrOfFrames + info.subtitles.nrOfFrames;
        // after headers follows the frame data
        info.frameData = data;
        return info;
    }

    IWRAM_FUNC auto HasMoreFrames(const Info &info, const Frame &previous) -> bool
    {
        return previous.index < static_cast<int32_t>(info.nrOfFrames - 1);
    }

    IWRAM_FUNC auto GetNextFrame(const Info &info, const Frame &previous) -> Frame
    {
        static_assert(sizeof(FrameHeader) % 4 == 0);
        Frame frame;
        const uint32_t *frameStart = nullptr;
        if (previous.index < 0 || previous.index >= static_cast<int32_t>(info.nrOfFrames - 1))
        {
            // read first frame
            frame.index = 0;
            frameStart = info.frameData;
        }
        else
        {
            // read next frame
            frame.index = previous.index + 1;
            frameStart = previous.data + previous.dataSize / 4;
        }
        auto frameHeader = reinterpret_cast<const FrameHeader *>(frameStart);
        frame.dataType = frameHeader->dataType;
        frame.dataSize = frameHeader->dataSize;
        frame.data = frameStart + sizeof(FrameHeader) / 4;
        return frame;
    }
}