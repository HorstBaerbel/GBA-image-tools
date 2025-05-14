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

EWRAM_DATA ALIGN(4) uint32_t ScratchPad[240 * 160 / 2 + 15000 / 4 + 1]; // scratch pad memory for decompression. ideally we would dynamically allocate this at the start of decoding

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
	Video::init(reinterpret_cast<const uint32_t *>(VIDEO_DATA), ScratchPad, sizeof(ScratchPad));
	Video::setClearColor(0);
	const auto &videoInfo = Video::getInfo();
	// print video info
	TUI::printf(0, 0, "Video decompression demo");
	TUI::printf(0, 2, "Frames: %d, Fps: %f", videoInfo.nrOfFrames, videoInfo.fps);
	TUI::printf(0, 3, "Size: %d kB", VIDEO_DATA_SIZE / 1024);
	TUI::printf(0, 4, "Resolution: %dx%d", videoInfo.width, videoInfo.height);
	TUI::printf(0, 5, "Bits / pixel: %d", videoInfo.bitsPerPixel);
	TUI::printf(0, 6, "Colors in colormap: %d", videoInfo.colorMapEntries);
	TUI::printf(0, 7, "Bits / color: %d", videoInfo.bitsPerColor);
	TUI::printf(0, 8, "Red-Blue swapped: %b", videoInfo.swappedRedBlue);
	TUI::printf(0, 9, "Video mem needed: %d Byte", videoInfo.videoMemoryNeeded);
	TUI::printf(0, 11, "Audio format: %d Hz, %d Bit", videoInfo.audioSampleRate, videoInfo.audioSampleBits);
	TUI::printf(0, 12, "Audio codec: %d", videoInfo.audioCodec);
	TUI::printf(0, 13, "Audio mem needed: %d Byte", videoInfo.audioMemoryNeeded);
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
	Video::play();
	do
	{
		Video::decodeAndBlitFrame((uint32_t *)VRAM);
		if (!Video::hasMoreFrames())
		{
			Video::stop();
			do
			{
				scanKeys();
				if (keysDown() & KEY_A)
				{
					break;
				}
			} while (true);
			Video::play();
		}
	} while (true);
	return 0;
}
