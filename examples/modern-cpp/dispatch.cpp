#include <cstddef>
#include <cstdint>

#include "print.h"

#ifdef NO_HEAP
/* The virtual destructor of Sensor makes the compiler generate a deleting destructor that calls
   operator delete, which calls free, which needs the heap. These two definitions keep the heap out
   of the image. They stop the program if anything ever deletes an object. */
void operator delete(void *) noexcept
{
    plat_exit(99);
}
void operator delete(void *, std::size_t) noexcept
{
    plat_exit(99);
}
#endif

struct Sensor {
    virtual std::int32_t read() = 0;
    virtual ~Sensor() = default;
};

struct Thermistor final : Sensor {
    std::int32_t read() override
    {
        return 25;
    }
};

/* The compiler does not know the type behind the reference, so it calls through the vtable. */
[[gnu::noinline]] std::int32_t through_base(Sensor &s)
{
    return s.read();
}

/* Thermistor is final, so the call can go directly to the function. */
[[gnu::noinline]] std::int32_t through_final(Thermistor &t)
{
    return t.read();
}

int main()
{
    Thermistor t;
    print_u32("sizeof(Thermistor)", sizeof t);
    print_u32("through base", static_cast<std::uint32_t>(through_base(t)));
    print_u32("through final", static_cast<std::uint32_t>(through_final(t)));
    plat_exit(0);
}
