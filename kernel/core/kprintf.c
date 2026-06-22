/* THUOS — minimal freestanding formatted output.
 * Supports: %s %c %d %i %u %x %X %p %% and a width-padded %0Nx for hex dumps.
 * This is intentionally small; it is not a full printf. */
#include "kprintf.h"
#include "vga.h"
#include "serial.h"
#include "gconsole.h"

static bool serial_mirror = true;

void k_serial_mirror(bool enabled) {
    serial_mirror = enabled;
}

void kputc(char c) {
    if (gcon_active()) gcon_putc(c);   /* graphical desktop console */
    else               vga_putc(c);    /* text-mode VGA            */
    if (serial_mirror) serial_write_char(c);
}

void kputs(const char *s) {
    while (*s) kputc(*s++);
}

static void print_uint(uint32_t value, uint32_t base, bool upper, int pad) {
    static const char *lower = "0123456789abcdef";
    static const char *upperd = "0123456789ABCDEF";
    const char *digits = upper ? upperd : lower;
    char buf[32];
    int i = 0;

    if (value == 0) {
        buf[i++] = '0';
    } else {
        while (value > 0) {
            buf[i++] = digits[value % base];
            value /= base;
        }
    }
    while (i < pad) buf[i++] = '0';
    while (i > 0) kputc(buf[--i]);
}

static void print_int(int32_t value) {
    if (value < 0) {
        kputc('-');
        print_uint((uint32_t)(-(int64_t)value), 10, false, 0);
    } else {
        print_uint((uint32_t)value, 10, false, 0);
    }
}

void kprintf(const char *fmt, ...) {
    __builtin_va_list args;
    __builtin_va_start(args, fmt);

    for (size_t i = 0; fmt[i]; i++) {
        if (fmt[i] != '%') {
            kputc(fmt[i]);
            continue;
        }

        i++;
        int pad = 0;
        /* optional zero-padded width, e.g. %08x */
        if (fmt[i] == '0') {
            i++;
            while (fmt[i] >= '0' && fmt[i] <= '9') {
                pad = pad * 10 + (fmt[i] - '0');
                i++;
            }
        }

        switch (fmt[i]) {
            case 's': {
                const char *s = __builtin_va_arg(args, const char *);
                kputs(s ? s : "(null)");
                break;
            }
            case 'c': {
                char c = (char)__builtin_va_arg(args, int);
                kputc(c);
                break;
            }
            case 'd':
            case 'i':
                print_int(__builtin_va_arg(args, int32_t));
                break;
            case 'u':
                print_uint(__builtin_va_arg(args, uint32_t), 10, false, pad);
                break;
            case 'x':
                print_uint(__builtin_va_arg(args, uint32_t), 16, false, pad);
                break;
            case 'X':
                print_uint(__builtin_va_arg(args, uint32_t), 16, true, pad);
                break;
            case 'p': {
                kputs("0x");
                print_uint((uint32_t)(uintptr_t)__builtin_va_arg(args, void *), 16, false, 8);
                break;
            }
            case '%':
                kputc('%');
                break;
            case '\0':
                __builtin_va_end(args);
                return;
            default:
                kputc('%');
                kputc(fmt[i]);
                break;
        }
    }

    __builtin_va_end(args);
}
