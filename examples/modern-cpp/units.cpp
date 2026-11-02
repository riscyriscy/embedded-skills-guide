#include <cstdint>

#include "print.h"

/* Two quantities that are both int32_t in C and can be mixed up without a warning. */
struct Millivolts {
    std::int32_t value;
};
struct Milliamps {
    std::int32_t value;
};

constexpr std::int32_t power_microwatts(Millivolts v, Milliamps i) noexcept
{
    return v.value * i.value; /* mV times mA is microwatts */
}

int main()
{
    print_u32("power in microwatts",
              static_cast<std::uint32_t>(power_microwatts(Millivolts{3300}, Milliamps{20})));
#ifdef SWAPPED
    print_u32("swapped",
              static_cast<std::uint32_t>(power_microwatts(Milliamps{20}, Millivolts{3300})));
#endif
    plat_exit(0);
}
