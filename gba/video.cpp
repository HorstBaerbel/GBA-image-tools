#include <gba_base.h>
#include <gba_input.h>
#include <gba_interrupt.h>
#include <gba_systemcalls.h>
#include <gba_timers.h>
#include <gba_video.h>

#include "dma.h"
#include "memory.h"
#include "tui.h"
#include "videodecoder.h"
#include "videoreader.h"

#include "data/data.h"

bool frameRequested = true;

void frameRequest()
{
	frameRequested = true;
}

EWRAM_DATA ALIGN(4) uint32_t ScratchPad[160 * 128 * 2 * 2 / 4]; // scratch pad memory for decompression. ideally we would dynamically allocate this

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
	// read file header
	const auto videoInfo = Video::GetInfo(VIDEO_DATA);
	// print video info
	TUI::printf(0, 0, "Frames: %d, Fps: %d", videoInfo.nrOfFrames, videoInfo.fps);
	TUI::printf(0, 1, "Size: %dx%d", videoInfo.width, videoInfo.height);
	TUI::printf(0, 2, "Bits / pixel: %d", videoInfo.bitsPerPixel);
	TUI::printf(0, 3, "Colors in colormap: %d", videoInfo.colorMapEntries);
	TUI::printf(0, 4, "Bits / color: %d", videoInfo.bitsInColorMap);
	TUI::printf(0, 5, "Memory needed: %d", videoInfo.maxMemoryNeeded);
	// set up timer to increase with frame interval
	irqSet(irqMASKS::IRQ_TIMER3, frameRequest);
	irqEnable(irqMASKS::IRQ_TIMER3);
	// Timer divider 2 == 256 -> 16*1024*1024 cycles/s / 256 = 65536/s
	REG_TM3CNT_H = TIMER_START | TIMER_IRQ | 2;
	// Timer interval = 1 / fps (where 65536 == 1s)
	REG_TM3CNT_L = 65536 - (65536 / videoInfo.fps);
	// start main loop
	Video::Frame frame{};
	do
	{
		// wait for the timer to signal a frame request
		while (!frameRequested)
		{
		};
		frameRequested = false;
		// read next frame from data
		frame = Video::GetNextFrame(videoInfo, frame);
		// uncompress frame
		//Video::decode(reinterpret_cast<uint8_t *>(VRAM), ScratchPad, videoInfo, frame);
	} while (true);
	return 0;
}
