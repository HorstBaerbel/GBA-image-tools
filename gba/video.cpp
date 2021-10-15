#include <gba_base.h>
#include <gba_video.h>
#include <gba_input.h>
#include <gba_timers.h>
#include <gba_interrupt.h>
#include <gba_systemcalls.h>

#include "dma.h"
#include "memory.h"
#include "tui.h"

int main()
{
	// set waitstates for GamePak ROM and EWRAM
	Memory::RegWaitCnt = Memory::WaitCntFast;
	Memory::RegWaitEwram = Memory::WaitEwramNormal;
	// start wall clock
	irqInit();
	// set up timer to increase time every ~5ms
	/*irqSet(irqMASKS::IRQ_TIMER3, updateTime);
	irqEnable(irqMASKS::IRQ_TIMER3);
	REG_TM3CNT_L = 65536 - TimerIncrement;
	REG_TM3CNT_H = TIMER_START | TIMER_IRQ | 2;*/
	// set up text UI
	TUI::setup();
	TUI::fillBackground(TUI::Color::Black);
	// start main loop
	do
	{
	} while (true);
	return 0;
}
