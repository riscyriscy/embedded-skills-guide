#include <stdint.h>

#include "FreeRTOS.h"
#include "message_buffer.h"
#include "stream_buffer.h"
#include "task.h"

#include "rt.h"

/* Bytes from a UART through a stream buffer. The handler of interrupt 0 stands for the receive
   interrupt: each interrupt takes the next byte of a script of 22 bytes and sends it with
   xStreamBufferSendFromISR(). The driver task makes one interrupt every 87 us, the time of a byte
   at 115200 baud. Part 1 reads with a trigger level of 1 and then of 8. Part 2 sends three lines
   through a message buffer. Part 3 sends the bytes to a buffer of 16 bytes that nobody reads. */

static const char script[] = "AT+MODE=2\r\nREADY\r\nOK\r\n";
#define SCRIPT_LEN (sizeof script - 1u)

static StreamBufferHandle_t rx;
static volatile uint32_t next_byte, accepted, dropped;
static volatile uint32_t wakes, total, largest;
static uint32_t wake_bytes[32];
static TaskHandle_t driver;

void Irq0_Handler(void)
{
    BaseType_t woken = pdFALSE;
    if (next_byte < SCRIPT_LEN) {
        char c = script[next_byte++];
        if (xStreamBufferSendFromISR(rx, &c, 1, &woken) == 1u) {
            accepted++;
        } else {
            dropped++;
        }
    }
    portYIELD_FROM_ISR(woken);
}

static void reader_task(void *arg)
{
    (void)arg;
    uint8_t buf[32];
    for (;;) {
        size_t n = xStreamBufferReceive(rx, buf, sizeof buf, pdMS_TO_TICKS(3));
        if (n == 0u) {
            break; /* nothing came in 3 ms */
        }
        if (wakes < 32u) {
            wake_bytes[wakes] = (uint32_t)n;
        }
        wakes++;
        total += (uint32_t)n;
        largest = n > largest ? (uint32_t)n : largest;
    }
    xTaskNotifyGive(driver);
    vTaskDelete(NULL);
}

static void feed(void)
{
    for (uint32_t i = 0; i < SCRIPT_LEN; i++) {
        rt_irq_pend(0);
        rt_busy_us(87);
    }
}

static void trigger_case(size_t trigger)
{
    rx = xStreamBufferCreate(64, trigger);
    next_byte = accepted = dropped = wakes = total = largest = 0;
    (void)xTaskCreate(reader_task, "reader", configMINIMAL_STACK_SIZE, NULL, 3, NULL);
    vTaskDelay(pdMS_TO_TICKS(2));
    feed();
    (void)ulTaskNotifyTake(pdTRUE, portMAX_DELAY);
    rt_say_u32("trigger level", (uint32_t)trigger);
    rt_say_u32("  bytes that the handler sent", accepted);
    rt_say_u32("  bytes that the task received", total);
    rt_say_u32("  wakes of the task", wakes);
    rt_say_u32("  most bytes in one wake", largest);
    for (uint32_t i = 0; wakes <= 4u && i < wakes; i++) {
        rt_say_u32("    bytes in this wake", wake_bytes[i]);
    }
    vStreamBufferDelete(rx);
}

static void show_space(const char *label, MessageBufferHandle_t mb)
{
    rt_say_u32(label, (uint32_t)xMessageBufferSpaceAvailable(mb));
}

static void driver_task(void *arg)
{
    (void)arg;
    trigger_case(1);
    trigger_case(8);

    /* Part 2: a message buffer keeps the boundaries of the lines. */
    MessageBufferHandle_t mb = xMessageBufferCreate(64);
    uint8_t line[32];
    show_space("message buffer, free bytes when empty", mb);
    (void)xMessageBufferSend(mb, &script[0], 11, 0);
    show_space("  free bytes after a message of 11 bytes", mb);
    (void)xMessageBufferSend(mb, &script[11], 7, 0);
    (void)xMessageBufferSend(mb, &script[18], 4, 0);
    show_space("  free bytes after messages of 7 and 4 bytes", mb);
    rt_say_u32("  xMessageBufferSend of 60 bytes returned",
               (uint32_t)xMessageBufferSend(mb, script, 60, 0));
    for (int i = 0; i < 3; i++) {
        size_t n = xMessageBufferReceive(mb, line, sizeof line, 0);
        rt_say_u32("  message received, bytes", (uint32_t)n);
    }
    show_space("  free bytes after the three receives", mb);
    vMessageBufferDelete(mb);

    /* Part 3: a buffer of 16 bytes and nobody reads. */
    rx = xStreamBufferCreate(16, 1);
    next_byte = accepted = dropped = 0;
    feed();
    rt_say("stream buffer of 16 bytes, the reader asleep");
    rt_say_u32("  bytes that the handler sent", accepted);
    rt_say_u32("  bytes that the handler lost", dropped);
    rt_say_u32("  bytes in the buffer", (uint32_t)xStreamBufferBytesAvailable(rx));
    rt_finish(0);
}

int main(void)
{
    rt_irq_setup(0, 192);
    (void)xTaskCreate(driver_task, "driver", configMINIMAL_STACK_SIZE * 2, NULL, 2, &driver);
    vTaskStartScheduler();
    for (;;) {
    }
}
