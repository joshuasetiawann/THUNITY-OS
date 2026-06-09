/* THUOS — PS/2 keyboard driver.
 * IRQ1 fills a small ring buffer with translated ASCII; the shell drains it. */
#include "keyboard.h"
#include "irq.h"
#include "io.h"

#define KBD_DATA   0x60
#define BUF_SIZE   128

static volatile char    buffer[BUF_SIZE];
static volatile uint32_t head;
static volatile uint32_t tail;
static bool shift_down;

/* US QWERTY, scancode set 1, unshifted. */
static const char scancode_map[128] = {
    0,  27, '1','2','3','4','5','6','7','8','9','0','-','=','\b',
    '\t','q','w','e','r','t','y','u','i','o','p','[',']','\n',
    0,  'a','s','d','f','g','h','j','k','l',';','\'','`',
    0,  '\\','z','x','c','v','b','n','m',',','.','/', 0,
    '*', 0,  ' ',
};

static const char scancode_map_shift[128] = {
    0,  27, '!','@','#','$','%','^','&','*','(',')','_','+','\b',
    '\t','Q','W','E','R','T','Y','U','I','O','P','{','}','\n',
    0,  'A','S','D','F','G','H','J','K','L',':','"','~',
    0,  '|','Z','X','C','V','B','N','M','<','>','?', 0,
    '*', 0,  ' ',
};

static void buffer_push(char c) {
    uint32_t next = (head + 1) % BUF_SIZE;
    if (next != tail) {        /* drop on overflow rather than corrupt */
        buffer[head] = c;
        head = next;
    }
}

static void keyboard_callback(registers_t *r) {
    (void)r;
    uint8_t scancode = inb(KBD_DATA);

    /* Shift make/break (0x2A/0x36 press, 0xAA/0xB6 release). */
    if (scancode == 0x2A || scancode == 0x36) { shift_down = true;  return; }
    if (scancode == 0xAA || scancode == 0xB6) { shift_down = false; return; }
    if (scancode & 0x80) return;            /* ignore other key releases */
    if (scancode >= 128)  return;

    char c = shift_down ? scancode_map_shift[scancode] : scancode_map[scancode];
    if (c) buffer_push(c);
}

void keyboard_init(void) {
    head = tail = 0;
    shift_down = false;
    irq_register_handler(1, keyboard_callback);
}

char keyboard_getchar(void) {
    while (head == tail) {
        __asm__ volatile("sti; hlt");  /* sleep until the next interrupt */
    }
    char c = buffer[tail];
    tail = (tail + 1) % BUF_SIZE;
    return c;
}
