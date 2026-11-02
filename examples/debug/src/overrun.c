#include <stdint.h>
#include <stdio.h>
#include <string.h>

/* The name of the link and the count of frames sent are neighbours in memory. */
static struct {
    char name[8];
    uint32_t frames_sent;
} link_state;

static void set_name(const char *name)
{
    strcpy(link_state.name, name);
}

static void send_frame(void)
{
    link_state.frames_sent++;
}

int main(void)
{
    for (int i = 0; i < 5; i++) {
        send_frame();
    }
    set_name("sensor-01");
    send_frame();
    printf("frames sent: %u (expected 6)\n", (unsigned)link_state.frames_sent);
    return 0;
}
