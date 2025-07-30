#include "time.h"

#include "memory/memory.h"

#include <gba_interrupt.h>
#include <gba_timers.h>

// The system clock is 16.78MHz (F=16*1024*1024 Hz), one cycle is thus approx. 59.59ns
constexpr uint16_t TimerDividerBits = 1;     // (0=F/1, 1=F/64, 2=F/256, 3=F/1024)
constexpr int32_t TimerReload = 65536 - 256; // 16*1024*1024 / 64 / 256 = 1024 -> 1 / 1024 = 0.9765 ms
constexpr int32_t TimeIncrement = 64;        // 0.9765 * 65536 =~ 64

namespace Time
{

    IWRAM_DATA static int32_t current = 0; // Time since timer was started

    // Called each timer tick to increase time value by 0.9765 ms
    IWRAM_FUNC auto timerTick() -> void
    {
        current += TimeIncrement;
    }

    IWRAM_FUNC auto start() -> void
    {
        irqSet(irqMASKS::IRQ_TIMER3, timerTick);
        irqEnable(irqMASKS::IRQ_TIMER3);
        REG_TM3CNT_L = TimerReload;
        REG_TM3CNT_H = TIMER_START | TIMER_IRQ | TimerDividerBits;
    }

    IWRAM_FUNC auto stop() -> void
    {
        REG_TM3CNT_H = 0;
        irqDisable(irqMASKS::IRQ_TIMER3);
    }

    IWRAM_FUNC auto now() -> int32_t
    {
        return current;
    }

} // namespace Time
