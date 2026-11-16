#include "crash.h"

#include <stdbool.h>
#include <stddef.h>

#define MAGIC           0x43525331u /* "CRS1" */
#define STATE_RUNNING   1u
#define STATE_REQUESTED 2u
#define STATE_CRASHED   3u

#define LINES    12u
#define LINE_LEN 64u

#define SCB_AIRCR (*(volatile uint32_t *)0xE000ED0Cu)

struct crash_record {
    uint32_t magic;
    uint32_t state;
    uint32_t code;
    uint32_t boots;
    uint32_t crashes;
    /* ~(magic ^ state ^ code ^ boots ^ crashes), so random memory is unlikely to pass */
    uint32_t check;
    uint32_t nlines;
    char line[LINES][LINE_LEN];
};

/* The only object in the section .persist. The linker script puts the section at PERSIST_ORIGIN. */
static struct crash_record rec __attribute__((section(".persist")));

extern const char _flash_begin[], _flash_end[], _ram_begin[], _ram_end[];

static uint32_t check_of(const struct crash_record *r)
{
    return ~(r->magic ^ r->state ^ r->code ^ r->boots ^ r->crashes);
}

static bool valid(void)
{
    return rec.magic == MAGIC && rec.check == check_of(&rec);
}

static void seal(uint32_t state)
{
    rec.magic = MAGIC;
    rec.state = state;
    rec.check = check_of(&rec);
}

static void reset_now(void) __attribute__((noreturn));
static void reset_now(void)
{
    __asm volatile("dsb" ::: "memory"); /* the record is in memory before the reset starts */
    SCB_AIRCR = 0x05FA0004u;            /* VECTKEY and SYSRESETREQ */
    __asm volatile("dsb" ::: "memory");
    for (;;) {
    }
}

/* ---- text for the record, with no library call ---- */

static unsigned col;

static void put(const char *s)
{
    if (rec.nlines >= LINES) {
        return;
    }
    char *line = rec.line[rec.nlines];
    while (*s != '\0' && col < LINE_LEN - 1u) {
        line[col++] = *s++;
    }
    line[col] = '\0';
}

static void put_num(uint32_t v, unsigned base, unsigned digits)
{
    char text[12];
    unsigned n = 0;
    do {
        text[n++] = "0123456789abcdef"[v % base];
        v /= base;
    } while (v != 0u || n < digits);
    char out[12];
    for (unsigned i = 0; i < n; i++) {
        out[i] = text[n - 1u - i];
    }
    out[n] = '\0';
    put(out);
}

static void end_line(void)
{
    if (rec.nlines < LINES) {
        rec.nlines++; /* a line counts only when it is complete */
    }
    col = 0;
    if (rec.nlines < LINES) {
        rec.line[rec.nlines][0] = '\0';
    }
}

/* The bootloader has no tasks. The kernel replaces this one (freertos_tasks_c_additions.h). */
__attribute__((weak)) void crash_each_task(crash_task_visit visit)
{
    (void)visit;
}

/* The application replaces this with the name of the running task. */
__attribute__((weak)) const char *crash_context(void)
{
    return "no tasks";
}

/* ---- reading the record ---- */

static enum reset_class classify(void)
{
    if (!valid()) {
        return RESET_POWER_ON;
    }
    switch (rec.state) {
    case STATE_REQUESTED:
        return RESET_REQUESTED;
    case STATE_CRASHED:
        return RESET_CRASH;
    default:
        return RESET_UNPLANNED;
    }
}

enum reset_class crash_peek(void)
{
    return classify();
}

enum reset_class crash_start(void)
{
    enum reset_class c = classify();
    if (c == RESET_POWER_ON) {
        rec.boots = 0;
        rec.crashes = 0;
        rec.code = 0;
        rec.nlines = 0;
    } else if (rec.nlines > LINES) {
        rec.nlines = LINES;
    }
    if (c != RESET_CRASH) {
        rec.nlines = 0;
    }
    rec.boots++;
    seal(STATE_RUNNING);
    return c;
}

unsigned crash_boots(void)
{
    return rec.boots;
}
unsigned crash_count(void)
{
    return rec.crashes;
}
uint32_t crash_code(void)
{
    return rec.code;
}
unsigned crash_lines(void)
{
    return rec.nlines;
}

const char *crash_line(unsigned i)
{
    rec.line[i][LINE_LEN - 1u] = '\0'; /* an end of string, whatever the line holds */
    return rec.line[i];
}

const char *crash_class_name(enum reset_class c)
{
    static const char *const names[] = {"power-on", "unplanned (watchdog, lock-up or brown-out)",
                                        "requested", "crash"};
    return names[c];
}

/* ---- the two ways to reset on purpose ---- */

void crash_request_reset(uint32_t code)
{
    rec.code = code;
    seal(STATE_REQUESTED);
    reset_now();
}

static const uint32_t *current_sp(void)
{
    uint32_t sp;
    __asm volatile("mov %0, sp" : "=r"(sp));
    return (const uint32_t *)sp;
}

/* Starts the record of a crash. Returns false when a crash is already being recorded. */
static bool begin(void)
{
    /* Nothing else runs, and nothing else writes the record. */
    __asm volatile("cpsid i" ::: "memory");

    static bool ongoing;
    if (ongoing) {
        return false;
    }
    ongoing = true;
    if (!valid()) {
        rec.boots = 0;
        rec.crashes = 0;
        rec.code = 0;
    }
    rec.nlines = 0;
    rec.line[0][0] = '\0';
    col = 0;
    rec.crashes++;
    /* Seal first, so that a second failure while the text is written is still reported. */
    seal(STATE_CRASHED);
    return true;
}

/* True for an address inside the flash or the RAM of the device. */
static bool plausible(const char *p)
{
    uint32_t a = (uint32_t)p;
    return (a >= (uint32_t)_ram_begin && a < (uint32_t)_ram_end) ||
           (a >= (uint32_t)_flash_begin && a < (uint32_t)_flash_end);
}

/* Appends up to four words of [from, to) that look like return addresses: an odd value inside the
   flash of the image. The scan is bounded, because the stack may be damaged. */
static void code_words(const uint32_t *from, const uint32_t *to)
{
    unsigned found = 0;
    for (unsigned i = 0; from + i < to && i < 256u && found < 4u; i++) {
        uint32_t w = from[i];
        if ((w & 1u) != 0u && w >= (uint32_t)_flash_begin && w < (uint32_t)_flash_end) {
            put(" 0x");
            put_num(w & ~1u, 16, 8);
            found++;
        }
    }
}

/* The stack pointer of the code that crashed. */
static const uint32_t *crash_sp;

/* One line for a task: its name and the code addresses between its stack pointer and the end of its
   stack. The running task has no saved pointer yet. When the crash happened on its own stack, the
   line "code on stack" has covered it. Otherwise it is read from the register PSP. */
static void task_stack(const char *name, const uint32_t *base, const uint32_t *top,
                       const uint32_t *end, int running)
{
    if (running) {
        if (crash_sp >= base && crash_sp <= end) {
            return;
        }
        uint32_t psp;
        __asm volatile("mrs %0, psp" : "=r"(psp));
        top = (const uint32_t *)psp;
    }
    if (!plausible(name) || !plausible((const char *)base) || !plausible((const char *)end) ||
        top < base || top > end) {
        return;
    }
    put("stack ");
    put(name);
    put(":");
    code_words(top, end + 1);
    end_line();
}

/* The task, the handler number if there is one, the code addresses on the stack that was in use,
   and then the same for every task. */
static void finish(const uint32_t *sp)
{
    uint32_t ipsr;
    __asm volatile("mrs %0, ipsr" : "=r"(ipsr));
    /* The kernel data that names the task may be the damage that caused the crash. */
    const char *task = crash_context();
    put("task ");
    put(plausible(task) ? task : "?");
    if (ipsr != 0u) {
        put(", handler ");
        put_num(ipsr, 10, 1);
    }
    end_line();

    crash_sp = sp;
    put("code on stack:");
    code_words(sp, sp + 64);
    end_line();

    crash_each_task(task_stack);
}

static const char *base_name(const char *path)
{
    const char *name = path;
    for (; *path != '\0'; path++) {
        if (*path == '/') {
            name = path + 1;
        }
    }
    return name;
}

void crash_system(const char *condition, const char *file, unsigned line)
{
    if (begin()) {
        put("fatal: ");
        put(condition);
        end_line();
        put("at ");
        put(base_name(file));
        put(":");
        put_num(line, 10, 1);
        end_line();
        finish(current_sp());
    }
    reset_now();
}

#define SCB_CFSR (*(volatile uint32_t *)0xE000ED28u)
#define SCB_HFSR (*(volatile uint32_t *)0xE000ED2Cu)

/* The hardware pushed r0-r3, r12, lr, pc and xpsr on the stack that was in use. */
void crash_fault(const uint32_t *frame) __attribute__((used, noreturn));
void crash_fault(const uint32_t *frame)
{
    if (begin()) {
        put("hard fault at pc 0x");
        put_num(frame[6], 16, 8);
        put(", lr 0x");
        put_num(frame[5], 16, 8);
        end_line();
        put("cfsr 0x");
        put_num(SCB_CFSR, 16, 8);
        put(", hfsr 0x");
        put_num(SCB_HFSR, 16, 8);
        end_line();
        finish(frame);
    }
    reset_now();
}

__attribute__((naked)) void HardFault_Handler(void)
{
    __asm volatile("tst lr, #4\n" /* which stack was in use */
                   "ite eq\n"
                   "mrseq r0, msp\n"
                   "mrsne r0, psp\n"
                   "b crash_fault\n");
}
