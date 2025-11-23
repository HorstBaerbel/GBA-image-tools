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
IWRAM_DATA ALIGN(4) uint32_t AudioSampleBuffer[2 * 6000 / 4];			 // Memory for audio sample double-buffer. Ideally we would dynamically allocate this at the start of decoding

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
	Media::Init(reinterpret_cast<const uint32_t *>(VIDEO_DATA), VideoScratchPad, sizeof(VideoScratchPad), 480, 2, AudioSampleBuffer, sizeof(AudioSampleBuffer));
	Media::SetClearColor(0);
	// print video info
	const auto &mediaInfo = Media::GetInfo();
	TUI::printf(0, 0, "Video decompression demo");
	TUI::printf(0, 2, "Frames: %d, Fps: %f", mediaInfo.video.nrOfFrames, mediaInfo.video.frameRateHz);
	TUI::printf(0, 3, "Size: %d kB", VIDEO_DATA_SIZE / 1024);
	TUI::printf(0, 4, "Resolution: %dx%d", mediaInfo.video.width, mediaInfo.video.height);
	TUI::printf(0, 5, "Bits / pixel: %d", mediaInfo.video.bitsPerPixel);
	TUI::printf(0, 6, "Colors in colormap: %d", mediaInfo.video.colorMapEntries);
	TUI::printf(0, 7, "Bits / color: %d", mediaInfo.video.bitsPerColor);
	TUI::printf(0, 8, "Color map frames: %d", mediaInfo.video.nrOfColorMapFrames);
	TUI::printf(0, 9, "Red-Blue swapped: %b", mediaInfo.video.swappedRedBlue);
	TUI::printf(0, 10, "Video mem needed: %d Byte", mediaInfo.video.memoryNeeded);
	TUI::printf(0, 12, "Audio samples: %d", mediaInfo.audio.nrOfSamples);
	TUI::printf(0, 13, "Audio sample rate: %d Hz", mediaInfo.audio.sampleRateHz);
	TUI::printf(0, 14, "Audio sample depth: %d-bit", mediaInfo.audio.sampleBits);
	TUI::printf(0, 15, "Audio channels: %d", mediaInfo.audio.channels);
	TUI::printf(0, 16, "Audio mem needed: %d Byte", mediaInfo.audio.memoryNeeded);
	TUI::printf(0, 19, "       Press A to play");
	// center video on screen
	Media::SetPosition((240 - mediaInfo.video.width) / 2, (160 - mediaInfo.video.height) / 2);
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
