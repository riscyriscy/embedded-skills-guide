/* The application: reports the last reset, then provokes the next one. */
#include <stdint.h>

#include "FreeRTOS.h"
#include "task.h"

#include "crash.h"
#include "platform.h"
#include "wdt.h"

const char *crash_context(void)
{
    return xTaskGetSchedulerState() == taskSCHEDULER_NOT_STARTED ? "none" : pcTaskGetName(NULL);
}

/* The kernel calls this when the last bytes of a task stack have changed. */
void vApplicationStackOverflowHook(TaskHandle_t task, char *name)
{
    (void)task;
    (void)name;
    crash_system("stack overflow", __FILE__, __LINE__);
}

void vApplicationMallocFailedHook(void)
{
    crash_system("heap exhausted", __FILE__, __LINE__);
}

void vApplicationTickHook(void)
{
}

static void say(const char *a, const char *b)
{
    char line[96];
    unsigned n = 0;
    for (; *a != '\0' && n < 40u; a++) {
        line[n++] = *a;
    }
    for (; *b != '\0' && n < 95u; b++) {
        line[n++] = *b;
    }
    line[n] = '\0';
    plat_puts(line);
}

/* In a product this sends the text to the log task, which writes it to the file system with the
   time of the new start. Here it goes to the console. */
static void report(enum reset_class why)
{
    say("application: reset class ", crash_class_name(why));
    if (why == RESET_REQUESTED) {
        plat_puts(crash_code() == 0x42u ? "    code 0x42" : "    another code");
    }
    for (unsigned i = 0; i < crash_lines(); i++) {
        say("    ", crash_line(i));
    }
}

static uint32_t deep(uint32_t depth)
{
    volatile uint8_t buffer[120];
    for (unsigned i = 0; i < sizeof buffer; i++) {
        buffer[i] = (uint8_t)(depth + i);
    }
    return depth < 2u ? deep(depth + 1u) + buffer[depth] : buffer[0];
}

static void wait_a_while(void)
{
    vTaskDelay(pdMS_TO_TICKS(1000));
}

static void logger(void *arg)
{
    (void)arg;
    for (;;) {
        wait_a_while();
    }
}

static void worker(void *arg)
{
    unsigned boot = (unsigned)(uintptr_t)arg;
    vTaskDelay(pdMS_TO_TICKS(3));
    switch (boot) {
    case 1: {
        unsigned queued = 5;
        configASSERT(queued < 4u); /* an assertion in application code */
        break;
    }
    case 2:
        (void)deep(0); /* more stack than the task has: the kernel hook fires at the next switch */
        vTaskDelay(pdMS_TO_TICKS(1));
        break;
    case 3:
        (void)*(volatile uint32_t *)0xA0000000u; /* an address with no memory */
        break;
    case 4:
        crash_request_reset(0x42u); /* a planned reset, for example after an update */
        break;
    case 5:
        wdt_start(0x00100000u); /* 41.9 ms a period, and nothing feeds it */
        vTaskDelay(portMAX_DELAY);
        break;
    default:
        plat_puts("application: done");
        plat_exit(0);
    }
    vTaskDelay(portMAX_DELAY);
}

int main(void)
{
    enum reset_class why = crash_start();
    unsigned boot = crash_boots();
    report(why);
    (void)xTaskCreate(worker, "worker", boot == 2u ? 96 : 128, (void *)(uintptr_t)boot, 1, NULL);
    (void)xTaskCreate(logger, "logger", 128, NULL, 1, NULL);
    vTaskStartScheduler();
    for (;;) {
    }
}
