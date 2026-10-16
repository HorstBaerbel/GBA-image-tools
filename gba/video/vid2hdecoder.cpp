#include "vid2hdecoder.h"

#include "audio/adpcm.h"
#include "audio/processingtype.h"
#include "audio/resampler.h"
#include "compression/bios.h"
#include "compression/lz4.h"
#include "compression/lz77.h"
#include "image/processingtype.h"
#include "memory/memory.h"
#include "sys/base.h"
#include "video/dxtv.h"

// Scratchpad swap strategy:
// 1 decompression stage:
//   - 0: decompress from frame.data to output buffer
// 2 decompression stages:
//   - 0: decompress from frame.data to scratchpad
//   - 1: decompress from scratchpad to output buffer
// 3 decompression stages:
//   - 0: decompress from frame.data to output buffer
//   - 1: decompress from output buffer to scratchpad
//   - 2: decompress from scratchpad to output buffer
// 4 decompression stages:
//   - 0: decompress from frame.data to scratchpad
//   - 1: decompress from scratchpad to output buffer
//   - 2: decompress from output buffer to scratchpad
//   - 3: decompress from scratchpad to output buffer
// So in the end the data always starts at output buffer

namespace Vid2hDecoder
{

    IWRAM_DATA Audio::LinearResamplerData m_resamplerData;
    IWRAM_DATA bool m_resampleAudio = false;

    IWRAM_FUNC auto DecodeVideo(uint32_t *scratchPad32, const uint32_t scratchPadSize8, uint8_t *vramPtr8, const uint32_t vramLineStride8, const IO::Vid2h::Info &info, const IO::Vid2h::Frame &frame) -> std::pair<const uint32_t *, uint32_t>
    {
        auto currentSrc32 = frame.data;
        uint32_t uncompressedSize8 = frame.dataSize; // if the frame data is initially uncompressed its size will be == frame data size
        uint32_t *currentDst32 = (info.nrOfVideoProcessings & 1) ? scratchPad32 + scratchPadSize8 / 4 - uncompressedSize8 / 4 : scratchPad32;
        // do decoding steps
        for (uint32_t pi = 0; pi < info.nrOfVideoProcessings; ++pi)
        {
            const auto isFinal = pi == (static_cast<uint32_t>(info.nrOfVideoProcessings) - 1);
            // get uncompressed size from data
            switch (info.video.processing[pi])
            {
            case Image::ProcessingType::CompressRLE:
                uncompressedSize8 = Compression::BIOSUnCompGetSize(currentSrc32);
                break;
            case Image::ProcessingType::CompressLZSS_10:
                uncompressedSize8 = Compression::BIOSUnCompGetSize(currentSrc32);
                break;
            case Image::ProcessingType::CompressLZ4_40:
                uncompressedSize8 = Compression::LZ4UnCompGetSize(currentSrc32);
                break;
            case Image::ProcessingType::CompressDXTV:
                uncompressedSize8 = Video::DXTVUnCompGetSize(currentSrc32);
                break;
            default:
                break;
            }
            // decode either to start or end of buffer
            currentDst32 = currentDst32 == scratchPad32 ? scratchPad32 + scratchPadSize8 / 4 - uncompressedSize8 / 4 : scratchPad32;
            // check wether destination is in VRAM (no 8-bit writes possible)
            const bool dstInVRAM = (((uint32_t)currentDst32) >= 0x05000000) && (((uint32_t)currentDst32) < 0x08000000);
            // reverse processing operation used in this stage
            switch (info.video.processing[pi])
            {
            case Image::ProcessingType::Uncompressed:
                Memory::memcpy32(currentDst32, currentSrc32, uncompressedSize8 / 4);
                break;
            case Image::ProcessingType::CompressRLE:
                dstInVRAM ? Compression::RLUnCompReadNormalWrite16bit(currentSrc32, currentDst32) : Compression::RLUnCompReadNormalWrite8bit(currentSrc32, currentDst32);
                break;
            case Image::ProcessingType::CompressLZSS_10:
                dstInVRAM ? Compression::LZ77UnCompWrite16bit(currentSrc32, currentDst32) : Compression::LZ77UnCompWrite8bit(currentSrc32, currentDst32);
                break;
            case Image::ProcessingType::CompressLZ4_40:
                Compression::LZ4UnCompWrite8bit(currentSrc32, currentDst32);
                break;
            case Image::ProcessingType::CompressDXTV:
                Video::DXTVUnCompWrite16bit(currentSrc32, currentDst32, (const uint32_t *)vramPtr8, vramLineStride8, info.video.width, info.video.height);
                break;
            default:
                return {currentDst32, uncompressedSize8};
            }
            // break if this was the last processing operation
            if (isFinal)
            {
                break;
            }
            // our old destination is the new source
            currentSrc32 = currentDst32;
        }
        return {currentDst32, uncompressedSize8};
    }

    auto DecodeAudioInit(const IO::Vid2h::Info &info, uint32_t dstRateHz) -> void
    {
        m_resampleAudio = false;
        if (info.audio.sampleRateHz != dstRateHz)
        {
            Audio::ResamplerInit(m_resamplerData, info.audio.sampleRateHz, dstRateHz);
            m_resampleAudio = true;
        }
    }

    IWRAM_FUNC auto DecodeAudio(uint8_t *outputBuffers8[2], uint32_t *scratchPad32, const IO::Vid2h::Info &info, const IO::Vid2h::Frame &frame) -> uint32_t
    {
        auto currentSrc8 = reinterpret_cast<const uint8_t *>(frame.data);
        uint32_t uncompressedSize8 = frame.dataSize; // if the frame data is initially uncompressed its size will be == frame data size
        auto scratchPad8 = reinterpret_cast<uint8_t *>(scratchPad32);
        uint8_t *currentDst8 = (info.nrOfAudioProcessings & 1) ? scratchPad8 : outputBuffers8[0];
        // do decoding steps
        for (uint32_t pi = 0; pi < info.nrOfAudioProcessings; ++pi)
        {
            const auto isFinal = pi == (static_cast<uint32_t>(info.nrOfAudioProcessings) - 1);
            // decode either to start or end of buffer
            currentDst8 = currentDst8 == scratchPad8 ? outputBuffers8[0] : scratchPad8;
            // reverse processing operation used in this stage
            switch (info.audio.processing[pi])
            {
            case Audio::ProcessingType::Uncompressed:
                Memory::memcpy32(currentDst8, currentSrc8, uncompressedSize8 / 4);
                break;
            case Audio::ProcessingType::CompressRLE:
                Compression::RLUnCompReadNormalWrite8bit(currentSrc8, currentDst8);
                uncompressedSize8 = Compression::BIOSUnCompGetSize(currentSrc8);
                break;
            case Audio::ProcessingType::CompressLZSS_10:
                Compression::LZ77UnCompWrite8bit(currentSrc8, currentDst8);
                uncompressedSize8 = Compression::BIOSUnCompGetSize(currentSrc8);
                break;
            case Audio::ProcessingType::CompressLZ4_40:
                Compression::LZ4UnCompWrite8bit(currentSrc8, currentDst8);
                uncompressedSize8 = Compression::LZ4UnCompGetSize(currentSrc8);
                break;
            case Audio::ProcessingType::CompressADPCM:
                uncompressedSize8 = Audio::ADPCMUnCompWrite8bit_8bit(currentSrc8, outputBuffers8, m_resampleAudio ? &m_resamplerData : nullptr);
                break;
            default:
                return uncompressedSize8;
            }
            // break if this was the last processing operation
            if (isFinal)
            {
                break;
            }
            // our old destination is the new source
            currentSrc8 = currentDst8;
        }
        return uncompressedSize8;
    }

    auto GetMaxAudioBufferSize(const IO::Vid2h::Info &info, uint32_t outputBits, uint32_t outputSampleRateHz) -> uint32_t
    {
        if (info.audio.sampleRateHz == outputSampleRateHz && info.audio.sampleBits == outputBits)
        {
            return info.audio.memoryNeeded;
        }
        uint32_t bufferSize = info.audio.memoryNeeded;
        // adjust for bit depth
        if (info.audio.sampleBits > outputBits)
        {
            bufferSize = ((bufferSize * outputBits) + (outputBits - 1)) / info.audio.sampleBits;
        }
        else if (info.audio.sampleBits < outputBits)
        {
            bufferSize = ((bufferSize * info.audio.sampleBits) + (info.audio.sampleBits - 1)) / outputBits;
        }
        // adjust for sample rate
        if (info.audio.sampleRateHz > outputSampleRateHz)
        {
            bufferSize = (bufferSize * info.audio.sampleRateHz) / outputSampleRateHz;
        }
        else if (info.audio.sampleRateHz < outputSampleRateHz)
        {
            bufferSize = (bufferSize * outputSampleRateHz) / info.audio.sampleRateHz;
        }
        return bufferSize;
    }

    IWRAM_FUNC auto DecodeSubtitles(const IO::Vid2h::Frame &frame) -> std::tuple<int32_t, int32_t, const char *>
    {
        auto src32 = frame.data;
        const uint32_t uncompressedSize8 = frame.dataSize;
        // make sure we have enough data
        if (uncompressedSize8 > (4 + 4 + 1))
        {
            // read times
            const int32_t startTime = *src32++;
            const int32_t endTime = *src32++;
            auto text = (const char *)src32;
            return {startTime, endTime, text};
        }
        return {0, 0, nullptr};
    }
}