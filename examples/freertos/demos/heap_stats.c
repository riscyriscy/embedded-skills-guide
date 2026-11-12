#include <stddef.h>
#include <stdint.h>

#include "FreeRTOS.h"
#include "task.h"

#include "rt.h"

/* The heap counters of heap_4. The program takes five blocks of 4000 bytes, frees two of them, and
   then asks for a block that is smaller than the free space but larger than every free block. The
   program has its own malloc failed hook, which counts the failure and lets pvPortMalloc return
   NULL. */

static volatile uint32_t failures;

void vApplicationMallocFailedHook(void)
{
    failures++;
}

static void stats(const char *label)
{
    HeapStats_t s;
    vPortGetHeapStats(&s);
    rt_say(label);
    rt_say_u32("  free bytes, xPortGetFreeHeapSize", (uint32_t)xPortGetFreeHeapSize());
    rt_say_u32("  lowest free bytes ever, xPortGetMinimumEverFreeHeapSize",
               (uint32_t)xPortGetMinimumEverFreeHeapSize());
    rt_say_u32("  largest free block", (uint32_t)s.xSizeOfLargestFreeBlockInBytes);
    rt_say_u32("  smallest free block", (uint32_t)s.xSizeOfSmallestFreeBlockInBytes);
    rt_say_u32("  free blocks", (uint32_t)s.xNumberOfFreeBlocks);
    rt_say_u32("  successful allocations", (uint32_t)s.xNumberOfSuccessfulAllocations);
    rt_say_u32("  successful frees", (uint32_t)s.xNumberOfSuccessfulFrees);
}

static void heap_task(void *arg)
{
    (void)arg;
    void *p[5];

    stats("when the program starts");
    for (int i = 0; i < 5; i++) {
        p[i] = pvPortMalloc(4000);
    }
    stats("after five blocks of 4000 bytes");
    vPortFree(p[1]);
    vPortFree(p[3]);
    stats("after freeing the second and the fourth");

    /* Take the tail of the heap, so that the only free blocks are the two holes. */
    HeapStats_t s;
    vPortGetHeapStats(&s);
    void *tail = pvPortMalloc(s.xSizeOfLargestFreeBlockInBytes - 64u);
    stats("after taking most of the largest block");
    (void)tail;
    size_t free_now = xPortGetFreeHeapSize();
    void *big = pvPortMalloc(6000);
    rt_say_u32("free bytes when the request of 6000 bytes was made", (uint32_t)free_now);
    rt_say_u32("pvPortMalloc(6000) returned NULL, 1 means yes", big == NULL);
    rt_say_u32("malloc failed hook calls", failures);
    stats("after the failed request");
    rt_finish(0);
}

int main(void)
{
    (void)xTaskCreate(heap_task, "heap", configMINIMAL_STACK_SIZE, NULL, 2, NULL);
    vTaskStartScheduler();
    for (;;) {
    }
}
