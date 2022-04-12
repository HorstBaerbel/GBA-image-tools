#include <gba_base.h>
#include <gba_input.h>
#include <gba_interrupt.h>
#include <gba_systemcalls.h>
#include <gba_timers.h>
#include <gba_video.h>

#include "base.h"
#include "memory.h"
#include "output.h"
#include "tui.h"
#include "videoplayer.h"

#include "data/data.h"

EWRAM_DATA ALIGN(4) uint32_t ScratchPad[240 * 160 / 2 + 20148 / 4]; // scratch pad memory for decompression. ideally we would dynamically allocate this at the start of decoding

int main()
{
	// set waitstates for GamePak ROM and EWRAM
	Memory::RegWaitCnt = Memory::WaitCntNormal;
	Memory::RegWaitEwram = Memory::WaitEwramNormal;
	// start wall clock
	irqInit();
	// set up text UI
	TUI::setup();
	TUI::fillBackground(TUI::Color::Black);
	// read file header
	Video::init(reinterpret_cast<const uint32_t *>(VIDEO_DATA), ScratchPad, sizeof(ScratchPad));
	const auto &videoInfo = Video::getInfo();
	// print video info
	TUI::printf(0, 0, "Frames: %d, Fps: %d", videoInfo.nrOfFrames, videoInfo.fps);
	TUI::printf(0, 1, "Size: %dx%d", videoInfo.width, videoInfo.height);
	TUI::printf(0, 2, "Bits / pixel: %d", videoInfo.bitsPerPixel);
	TUI::printf(0, 3, "Colors in colormap: %d", videoInfo.colorMapEntries);
	TUI::printf(0, 4, "Bits / color: %d", videoInfo.bitsInColorMap);
	TUI::printf(0, 5, "Memory needed: %d", videoInfo.maxMemoryNeeded);
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
	int32_t maxFrameTimeMs = 0;
	Video::play();
	do
	{
		// start benchmark timer
		REG_TM2CNT_L = 0;
		REG_TM2CNT_H = TIMER_START | 2;
		Video::decodeFrame();
		Video::blitFrameTo((uint32_t *)VRAM);
		// end benchmark timer
		REG_TM2CNT_H = 0;
		auto durationMs = static_cast<int32_t>(REG_TM2CNT_L) * 1000;
		if (maxFrameTimeMs < durationMs)
		{
			maxFrameTimeMs = durationMs;
			Debug::printf("Max. frame time: %f ms", durationMs);
		}
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
