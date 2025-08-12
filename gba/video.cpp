#include <gba_base.h>
#include <gba_input.h>
#include <gba_interrupt.h>
#include <gba_video.h>

#include "base.h"
#include "memory.h"
#include "output.h"
#include "tui.h"
#include "videoplayer.h"

#include "data/video.h"

EWRAM_DATA ALIGN(4) uint32_t VideoScratchPad[240 * 160 / 2 + 16000 / 4]; // Scratch pad memory for video decompression. Ideally we would dynamically allocate this at the start of decoding
IWRAM_DATA ALIGN(4) uint32_t AudioSampleBuffer[2 * 3000 / 4];			 // Memory for audio sample double-buffer. Ideally we would dynamically allocate this at the start of decoding

int main()
{
	// set waitstates for GamePak ROM and EWRAM
	Memory::RegWaitCnt = Memory::WaitCntFast;
	Memory::RegWaitEwram = Memory::WaitEwramNormal;
	// start wall clock
	irqInit();
	// set up text UI
	TUI::setup();
	TUI::fillBackground(TUI::Color::Black);
	// set up video system, clear color and read file header
	Media::Init(reinterpret_cast<const uint32_t *>(VIDEO_DATA), VideoScratchPad, sizeof(VideoScratchPad), AudioSampleBuffer, sizeof(AudioSampleBuffer));
	Media::SetClearColor(0);
	// print video info
	const auto &videoInfo = Media::GetInfo();
	TUI::printf(0, 0, "Video decompression demo");
	TUI::printf(0, 2, "Frames: %d, Fps: %f", videoInfo.video.nrOfFrames, videoInfo.video.frameRateHz);
	TUI::printf(0, 3, "Size: %d kB", VIDEO_DATA_SIZE / 1024);
	TUI::printf(0, 4, "Resolution: %dx%d", videoInfo.video.width, videoInfo.video.height);
	TUI::printf(0, 5, "Bits / pixel: %d", videoInfo.video.bitsPerPixel);
	TUI::printf(0, 6, "Colors in colormap: %d", videoInfo.video.colorMapEntries);
	TUI::printf(0, 7, "Bits / color: %d", videoInfo.video.bitsPerColor);
	TUI::printf(0, 8, "Color map frames: %d", videoInfo.video.nrOfColorMapFrames);
	TUI::printf(0, 9, "Red-Blue swapped: %b", videoInfo.video.swappedRedBlue);
	TUI::printf(0, 10, "Video mem needed: %d Byte", videoInfo.video.memoryNeeded);
	TUI::printf(0, 12, "Audio samples: %d", videoInfo.audio.nrOfSamples);
	TUI::printf(0, 13, "Audio sample rate: %d Hz", videoInfo.audio.sampleRateHz);
	TUI::printf(0, 14, "Audio sample depth: %d-bit", videoInfo.audio.sampleBits);
	TUI::printf(0, 15, "Audio channels: %d", videoInfo.audio.channels);
	TUI::printf(0, 16, "Audio mem needed: %d Byte", videoInfo.audio.memoryNeeded);
	TUI::printf(0, 19, "       Press A to play");
	// wait for keypress
	do
	{
		scanKeys();
		if (keysDown() & KEY_A)
		{
			break;
		}
	} while (true);
	// switch video mode to 240x160x2
	REG_DISPCNT = MODE_3 | BG2_ON;
	// start main loop
	Media::Play();
	do
	{
		Media::DecodeAndPlay();
		if (!Media::HasMoreFrames())
		{
			Media::Stop();
			do
			{
				scanKeys();
				if (keysDown() & KEY_A)
				{
					break;
				}
			} while (true);
			Media::Play();
		}
	} while (true);
	return 0;
}
