#include "time.h"

#include "memory/memory.h"

#include <gba_interrupt.h>
#include <gba_timers.h>

// The system clock is 16.78MHz (F=16*1024*1024 Hz), one cycle is thus approx. 59.59ns
constexpr uint16_t TimerDividerBits = 2; // (0=F/1, 1=F/64, 2=F/256, 3=F/1024)
constexpr int32_t TimerIncrement = 164;  // 65536/164=400 -> 1/400=2.5ms

namespace Time
{

    IWRAM_DATA static int32_t current = 0; // Time since timer was started

    // Called each timer tick to increase time value
    IWRAM_FUNC auto timerTick() -> void
    {
        current += TimerIncrement;
    }

    auto start() -> void
    {
        irqSet(irqMASKS::IRQ_TIMER3, timerTick);
        irqEnable(irqMASKS::IRQ_TIMER3);
        REG_TM3CNT_L = 65536 - TimerIncrement;
        REG_TM3CNT_H = TIMER_START | TIMER_IRQ | TimerDividerBits;
    }

    auto stop() -> void
    {
        REG_TM3CNT_H = 0;
        irqDisable(irqMASKS::IRQ_TIMER3);
    }

    IWRAM_FUNC auto now() -> int32_t
    {
        return current;
    }

} // namespace Time
