#include <gba_base.h>
#include <gba_video.h>
#include <gba_input.h>
#include <gba_timers.h>
#include <gba_interrupt.h>
#include <gba_systemcalls.h>

#include "dma.h"
#include "memory.h"

int main()
{
	// set default waitstates for GamePak ROM and EWRAM
	Memory::RegWaitCnt = Memory::WaitCntFast;
	Memory::RegWaitEwram = Memory::WaitEwramNormal;
	// set graphics to mode 0 and enable background 2
	REG_DISPCNT = MODE_3 | BG2_ON;
	// start wall clock
	irqInit();
	// set up timer to increase time every ~5ms
	irqSet(irqMASKS::IRQ_TIMER3, updateTime);
	irqEnable(irqMASKS::IRQ_TIMER3);
	REG_TM3CNT_L = 65536 - TimerIncrement;
	REG_TM3CNT_H = TIMER_START | TIMER_IRQ | 2;
	// start main loop
	do
	{
	} while (true);
	return 0;
}
