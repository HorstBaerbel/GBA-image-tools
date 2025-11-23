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
	TUI::printf(0, 0, "Media decompression demo");
	TUI::printf(0, 2, "File size: %d kB", VIDEO_DATA_SIZE / 1024);
	TUI::printf(0, 4, "Video:");
	TUI::printf(0, 5, "Resolution: %dx%d @ %d bpp", mediaInfo.video.width, mediaInfo.video.height, mediaInfo.video.bitsPerPixel);
	TUI::printf(0, 6, "Frames: %d, Fps: %f", mediaInfo.video.nrOfFrames, mediaInfo.video.frameRateHz);
	TUI::printf(0, 7, "Duration: %f s", int32_t((uint64_t(mediaInfo.video.nrOfFrames) << 32) / mediaInfo.video.frameRateHz));
	TUI::printf(0, 8, "Colormap size: %d @ %d bpp", mediaInfo.video.colorMapEntries, mediaInfo.video.bitsPerColor);
	TUI::printf(0, 9, "Color map frames: %d", mediaInfo.video.nrOfColorMapFrames);
	TUI::printf(0, 10, "Red-Blue swapped: %b", mediaInfo.video.swappedRedBlue);
	TUI::printf(0, 11, "Memory needed: %d Byte", mediaInfo.video.memoryNeeded);
	TUI::printf(0, 13, "Audio:");
	TUI::printf(0, 14, "Channels: %d, Samples: %d", mediaInfo.audio.channels, mediaInfo.audio.nrOfSamples);
	TUI::printf(0, 15, "Rate: %d Hz, Depth: %d bit", mediaInfo.audio.sampleRateHz, mediaInfo.audio.sampleBits);
	TUI::printf(0, 16, "Duration: %f s", int32_t((uint64_t(mediaInfo.audio.nrOfSamples) << 16) / mediaInfo.audio.sampleRateHz));
	TUI::printf(0, 17, "Memory needed: %d Byte", mediaInfo.audio.memoryNeeded);
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
