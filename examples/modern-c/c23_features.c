#include <stddef.h>
#include <stdint.h>

#include "print.h"

/* constexpr: a typed constant that the compiler evaluates, with no macro and no memory. */
constexpr uint32_t sample_rate_hz = 8'000;    /* digit separators make a long number readable */
constexpr uint8_t status_ready = 0b0000'0001; /* binary literals */
constexpr size_t window = 8;

/* The underlying type of an enum: one byte instead of four. */
enum state : uint8_t { STATE_IDLE, STATE_RUNNING, STATE_FAULT };
enum plain { PLAIN_IDLE, PLAIN_RUNNING };

struct record {
    enum state state;
    enum plain plain;
};

/* [[nodiscard]]: a caller that ignores the result gets a warning. */
[[nodiscard]] static bool push_sample(int16_t *buffer, size_t used)
{
    if (used >= window) {
        return false;
    }
    buffer[used] = 1;
    return true;
}

static const char *name_of(enum state s)
{
    switch (s) {
    case STATE_IDLE:
        [[fallthrough]];
    case STATE_RUNNING:
        return "ok";
    case STATE_FAULT:
        return "fault";
    }
    unreachable(); /* the switch covers every value */
}

int main(void)
{
    [[maybe_unused]] const uint32_t spare = 3; /* no unused variable warning */
    int16_t samples[window] = {};              /* an empty initializer clears the array */
    int16_t *none = nullptr;                   /* nullptr has its own type */
    typeof(samples[0]) first = 7;              /* typeof names the type of an expression */
    static_assert(sizeof(samples) == 16);      /* the message is optional */

    if (!push_sample(samples, 0)) {
        plat_puts("push failed");
    }
    print_u32("sample rate", sample_rate_hz);
    print_u32("status ready", status_ready);
    print_u32("sizeof(enum state)", sizeof(enum state));
    print_u32("sizeof(enum plain)", sizeof(enum plain));
    print_u32("sizeof(struct record)", sizeof(struct record));
    print_u32("first sample", (uint32_t)first);
    print_u32("none is null", none == nullptr);
    plat_puts(name_of(STATE_FAULT));
    plat_exit(0);
}
