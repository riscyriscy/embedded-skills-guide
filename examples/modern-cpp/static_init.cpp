#include <cstdint>

#include "print.h"

volatile std::int32_t seed = 42; /* read at run time, so that nothing below can be folded */

/* Dynamic initialization: the constructor must run before main. */
struct RuntimeConfig {
    std::int32_t value;
    RuntimeConfig() : value(seed)
    {
    }
};
RuntimeConfig runtime_config;

/* Constant initialization: the value is in the image and needs no code to run. */
struct FixedConfig {
    std::int32_t value;
    constexpr FixedConfig() : value(42)
    {
    }
};
constinit FixedConfig fixed_config;

int main()
{
    print_u32("runtime config", static_cast<std::uint32_t>(runtime_config.value));
    print_u32("fixed config", static_cast<std::uint32_t>(fixed_config.value));
    plat_exit(0);
}
