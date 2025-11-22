#include "vid2hdecoder.h"

#include "compression/lz77.h"
#include "video/dxtv.h"
#include "audio/adpcm.h"
#include "memory/memory.h"
#include "sys/base.h"
#include "sys/decompress.h"

#include "image/processingtype.h"

namespace Media
{

    IWRAM_FUNC auto DecodeVideo(uint32_t *scratchPad, uint32_t scratchPadSize, const IO::Vid2h::Info &info, const IO::Vid2h::Frame &frame) -> std::pair<const uint32_t *, uint32_t>
    {
        auto currentSrc = frame.data;
        uint32_t uncompressedSize = frame.dataSize; // if the frame data is initially uncompressed its size will be == frame data size
        uint32_t *currentDst = nullptr;
        // do decoding steps
        for (uint32_t pi = 0; pi < sizeof(info.video.processing); ++pi)
        {
            // this is the final operation either if we don't have any more steps, the current step is just a copy, or the next step is invalid
            const auto processingType = info.video.processing[pi] == Image::ProcessingType::Invalid ? Image::ProcessingType::Uncompressed : info.video.processing[pi];
            const auto isFinal = (pi >= 3) || (processingType == Image::ProcessingType::Uncompressed) || (info.video.processing[pi + 1] == Image::ProcessingType::Invalid);
            // if we're reading from start of scratchpad, write to the end and vice versa
            currentDst = currentSrc == scratchPad ? scratchPad + scratchPadSize / 8 : scratchPad;
            // check wether destination is in VRAM (no 8-bit writes possible)
            const bool dstInVRAM = (((uint32_t)currentDst) >= 0x05000000) && (((uint32_t)currentDst) < 0x08000000);
            // reverse processing operation used in this stage
            switch (processingType)
            {
            case Image::ProcessingType::Uncompressed:
                Memory::memcpy32(currentDst, currentSrc, uncompressedSize / 4);
                break;
            case Image::ProcessingType::CompressLZ10:
                dstInVRAM ? Decompress::LZ77UnCompWrite16bit(currentSrc, currentDst) : Decompress::LZ77UnCompWrite8bit(currentSrc, currentDst);
                uncompressedSize = Decompress::BIOSUnCompGetSize(currentSrc);
                break;
            case Image::ProcessingType::CompressRLE:
                dstInVRAM ? BIOS::RLUnCompReadNormalWrite16bit(currentSrc, currentDst) : BIOS::RLUnCompReadNormalWrite8bit(currentSrc, currentDst);
                uncompressedSize = Decompress::BIOSUnCompGetSize(currentSrc);
                break;
            case Image::ProcessingType::CompressDXTV:
                Dxtv::UnCompWrite16bit(currentSrc, currentDst, (const uint32_t *)VRAM, 480, info.video.width, info.video.height);
                uncompressedSize = Dxtv::UnCompGetSize(currentSrc);
                break;
            default:
                return {currentDst, uncompressedSize};
            }
            // break if this was the last processing operation
            if (isFinal)
            {
                break;
            }
            // our old destination is the new source
            currentSrc = currentDst;
        }
        return {currentDst, uncompressedSize};
    }

    IWRAM_FUNC auto DecodeAudio(uint32_t *scratchPad, uint32_t scratchPadSize, const IO::Vid2h::Info &info, const IO::Vid2h::Frame &frame) -> std::pair<const uint32_t *, uint32_t>
    {
        auto currentSrc = frame.data;
        uint32_t uncompressedSize = frame.dataSize; // if the frame data is initially uncompressed its size will be == frame data size
        uint32_t *currentDst = nullptr;
        // do decoding steps
        for (uint32_t pi = 0; pi < sizeof(info.audio.processing); ++pi)
        {
            // this is the final operation either if we don't have any more steps, the current step is just a copy, or the next step is invalid
            const auto processingType = info.audio.processing[pi] == Audio::ProcessingType::Invalid ? Audio::ProcessingType::Uncompressed : info.audio.processing[pi];
            const auto isFinal = (pi >= 3) || (processingType == Audio::ProcessingType::Uncompressed) || (info.audio.processing[pi + 1] == Audio::ProcessingType::Invalid);
            // if we're reading from start of scratchpad, write to the end and vice versa
            currentDst = currentSrc == scratchPad ? scratchPad + scratchPadSize / 8 : scratchPad;
            // reverse processing operation used in this stage
            switch (processingType)
            {
            case Audio::ProcessingType::Uncompressed:
                Memory::memcpy32(currentDst, currentSrc, uncompressedSize / 4);
                break;
            case Audio::ProcessingType::CompressLZ10:
                Decompress::LZ77UnCompWrite8bit(currentSrc, currentDst);
                uncompressedSize = Decompress::BIOSUnCompGetSize(currentSrc);
                break;
            case Audio::ProcessingType::CompressRLE:
                BIOS::RLUnCompReadNormalWrite8bit(currentSrc, currentDst);
                uncompressedSize = Decompress::BIOSUnCompGetSize(currentSrc);
                break;
            case Audio::ProcessingType::CompressADPCM:
                Adpcm::UnCompWrite32bit<8>(currentSrc, uncompressedSize, currentDst);
                uncompressedSize = Adpcm::UnCompGetSize<8>(currentSrc);
                break;
            default:
                return {currentDst, uncompressedSize};
            }
            // break if this was the last processing operation
            if (isFinal)
            {
                break;
            }
            // our old destination is the new source
            currentSrc = currentDst;
        }
        return {currentDst, uncompressedSize};
    }
}