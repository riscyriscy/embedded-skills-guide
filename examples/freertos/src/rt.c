#include "rt.h"

#include "FreeRTOS.h"
#include "task.h"

#include <stddef.h>

#include "platform.h"

#define SYST_RVR  (*(volatile uint32_t *)0xE000E014u)
#define SYST_CVR  (*(volatile uint32_t *)0xE000E018u)
#define SCB_ICSR  (*(volatile uint32_t *)0xE000ED04u)
#define NVIC_ISER ((volatile uint32_t *)0xE000E100u)
#define NVIC_ISPR ((volatile uint32_t *)0xE000E200u)
#define NVIC_IPR  ((volatile uint8_t *)0xE000E400u)

uint32_t rt_now_us(void)
{
    TickType_t ticks;
    uint32_t value;
    do {
        ticks = xTaskGetTickCount();
        value = SYST_CVR;
    } while (ticks != xTaskGetTickCount());
    uint32_t elapsed = (SYST_RVR - value) / (configCPU_CLOCK_HZ / 1000000u);
    return (uint32_t)ticks * 1000u + elapsed;
}

static unsigned append(char *line, unsigned n, const char *text)
{
    while (*text != '\0' && n < 90u) {
        line[n++] = *text++;
    }
    return n;
}

static unsigned append_u32(char *line, unsigned n, uint32_t value)
{
    char digits[11];
    unsigned d = 0;
    do {
        digits[d++] = (char)('0' + value % 10u);
        value /= 10u;
    } while (value != 0u);
    while (d > 0u) {
        line[n++] = digits[--d];
    }
    return n;
}

static void emit(const char *label, int has_value, uint32_t value, const char *text)
{
    char line[100];
    unsigned n = append(line, 0, "t=");
    uint32_t ms = rt_now_us() / 1000u;
    if (ms < 100u) {
        line[n++] = ' ';
    }
    if (ms < 10u) {
        line[n++] = ' ';
    }
    n = append_u32(line, n, ms);
    n = append(line, n, " ms  ");
    n = append(line, n, label);
    if (has_value != 0) {
        n = append(line, n, ": ");
        n = append_u32(line, n, value);
    }
    if (text != NULL) {
        n = append(line, n, ": ");
        n = append(line, n, text);
    }
    line[n] = '\0';
    if ((SCB_ICSR & 0x1FFu) != 0u) {
        plat_puts(line); /* in an interrupt handler or a hook that the kernel calls from one */
    } else {
        taskENTER_CRITICAL();
        plat_puts(line);
        taskEXIT_CRITICAL();
    }
}

void rt_say(const char *text)
{
    emit(text, 0, 0, NULL);
}

void rt_say_str(const char *label, const char *text)
{
    emit(label, 0, 0, text);
}

void rt_say_u32(const char *label, uint32_t value)
{
    emit(label, 1, value, NULL);
}

void rt_busy_us(uint32_t microseconds)
{
    uint32_t last = rt_now_us();
    uint32_t spent = 0;
    while (spent < microseconds) {
        uint32_t now = rt_now_us();
        uint32_t step = now - last;
        last = now;
        /* A step of many microseconds means that another task ran in between. */
        if (step < 40u) {
            spent += step;
        }
    }
}

void rt_finish(int status)
{
    plat_exit(status);
}

void rt_irq_setup(uint32_t irq, uint8_t priority)
{
    NVIC_IPR[irq] = priority;
    NVIC_ISER[irq / 32u] = 1u << (irq % 32u);
}

void rt_irq_pend(uint32_t irq)
{
    NVIC_ISPR[irq / 32u] = 1u << (irq % 32u);
}

void rt_assert_failed(const char *file, int line)
{
    rt_say_str("assertion failed in", file);
    rt_say_u32("assertion failed at line", (uint32_t)line);
    plat_exit(2);
}

__attribute__((weak)) void vApplicationMallocFailedHook(void)
{
    rt_say("heap exhausted");
    plat_exit(3);
}

void vApplicationStackOverflowHook(TaskHandle_t task, char *name)
{
    (void)task;
    rt_say_str("stack overflow in task", name);
    plat_exit(4);
}

/* A program that does not need the tick hook gets this empty one. */
__attribute__((weak)) void vApplicationTickHook(void)
{
}
