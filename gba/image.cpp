#include <gba_base.h>
#include <gba_input.h>
#include <gba_interrupt.h>
#include <gba_systemcalls.h>
#include <gba_timers.h>
#include <gba_video.h>

#include "base.h"
#include "compression/lz77.h"
#include "image/dxt.h"
#include "memory.h"
#include "output.h"
#include "tui.h"

#include "data/images_dxt.h"

IWRAM_DATA ALIGN(4) uint32_t ScratchPad[IMAGES_DXT_DECOMPRESSION_BUFFER_SIZE / 4]; // scratch pad memory for decompression. ideally we would dynamically allocate this at the start of decoding

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
	TUI::printf(0, 8, "   DXT1 decompression demo");
	TUI::printf(0, 10, "       Press A to skip");
	TUI::printf(0, 11, "        to next image");
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
	uint32_t imageIndex = 0;
	REG_DISPCNT = MODE_3 | BG2_ON;
	do
	{
		// start benchmark timer
		REG_TM3CNT_L = 0;
		REG_TM3CNT_H = TIMER_START | 2;
		// decode and blit new image
		Compression::LZ77UnCompWrite16bit(&IMAGES_DXT_DATA[IMAGES_DXT_DATA_START[imageIndex]], ScratchPad);
		DXT::UnCompWrite16bit<240>((uint16_t *)VRAM, (uint16_t *)ScratchPad, 240, 160);
		// end benchmark timer
		REG_TM3CNT_H = 0;
		auto durationMs = static_cast<int32_t>(REG_TM3CNT_L) * 1000;
		Debug::printf("Decoding + display time: %f ms", durationMs);
		// wait for next key press
		do
		{
			scanKeys();
			if (keysDown() & KEY_A)
			{
				break;
			}
		} while (true);
		imageIndex = (imageIndex + 1) % IMAGES_DXT_NR_OF_IMAGES;
	} while (true);
	return 0;
}
