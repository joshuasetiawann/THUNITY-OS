/* THUOS — PIT timer driver. Programs channel 0 to a fixed frequency and
 * counts ticks on IRQ0, giving the kernel a notion of uptime. */
#include "pit.h"
#include "irq.h"
#include "io.h"

#define PIT_CHANNEL0 0x40
#define PIT_COMMAND  0x43
#define PIT_BASE_HZ  1193182u

static volatile uint32_t ticks;
static uint32_t freq_hz = 100;

static void pit_callback(registers_t *r) {
    (void)r;
    ticks++;
}

void pit_init(uint32_t frequency_hz) {
    if (frequency_hz == 0) frequency_hz = 100;
    freq_hz = frequency_hz;
    ticks = 0;

    uint32_t divisor = PIT_BASE_HZ / frequency_hz;
    if (divisor > 0xFFFF) divisor = 0xFFFF;

    outb(PIT_COMMAND, 0x36); /* channel 0, lo/hi byte, mode 3 (square wave) */
    outb(PIT_CHANNEL0, (uint8_t)(divisor & 0xFF));
    outb(PIT_CHANNEL0, (uint8_t)((divisor >> 8) & 0xFF));

    irq_register_handler(0, pit_callback);
}

uint32_t pit_ticks(void) {
    return ticks;
}

uint32_t pit_frequency(void) {
    return freq_hz;
}

uint32_t pit_seconds(void) {
    return freq_hz ? (ticks / freq_hz) : 0;
}
