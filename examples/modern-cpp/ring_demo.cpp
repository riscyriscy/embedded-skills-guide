#include "print.h"
#include "ringbuf.hpp"

/* The mean of the samples that a buffer holds, as a function that the compiler can run. */
template <typename Buffer> constexpr std::int32_t mean(const Buffer &buffer) noexcept
{
    std::int32_t sum = 0;
    for (std::size_t i = 0; i < buffer.count(); ++i) {
        sum += *buffer.get(i);
    }
    return buffer.count() == 0 ? 0 : sum / static_cast<std::int32_t>(buffer.count());
}

constexpr std::int32_t mean_after_twenty_pushes() noexcept
{
    RingBuffer<std::int16_t, 16> buffer;
    for (std::int16_t i = 1; i <= 20; ++i) {
        buffer.push(static_cast<std::int16_t>(i * 10));
    }
    return mean(buffer);
}

/* The test of firmware logic runs while the program is compiled. A wrong value stops the build. */
static_assert(mean_after_twenty_pushes() == 125, "the mean of the newest 16 samples is 125");

int main()
{
    RingBuffer<std::int16_t, 16> buffer;
    for (std::int16_t i = 1; i <= 20; ++i) {
        buffer.push(static_cast<std::int16_t>(i * 10));
    }
    print_u32("count", buffer.count());
    print_u32("oldest", static_cast<std::uint32_t>(*buffer.get(0)));
    print_u32("mean", static_cast<std::uint32_t>(mean(buffer)));
    print_u32("out of range is empty", !buffer.get(16).has_value());
    plat_exit(0);
}
