#include <cstdint>

#include "print.h"

/* Disables interrupts for the lifetime of the object and restores the previous state on every way
   out of the scope: a return, a break or a later change that adds one. */
class IrqGuard
{
  public:
    IrqGuard() noexcept
    {
        asm volatile("mrs %0, primask\n\tcpsid i" : "=r"(saved_) : : "memory");
    }
    ~IrqGuard()
    {
        asm volatile("msr primask, %0" : : "r"(saved_) : "memory");
    }
    IrqGuard(const IrqGuard &) = delete;
    IrqGuard &operator=(const IrqGuard &) = delete;

  private:
    std::uint32_t saved_;
};

static std::uint32_t primask()
{
    std::uint32_t value;
    asm volatile("mrs %0, primask" : "=r"(value));
    return value;
}

static std::uint32_t shared_counter;

static bool increment_below(std::uint32_t limit)
{
    IrqGuard guard;
    if (shared_counter >= limit) {
        return false; /* the guard restores the interrupt state here */
    }
    ++shared_counter;
    return true; /* and here */
}

int main()
{
    print_u32("primask before", primask());
    {
        IrqGuard guard;
        print_u32("primask inside", primask());
    }
    print_u32("primask after", primask());
    print_u32("first increment", increment_below(1));
    print_u32("second increment", increment_below(1));
    print_u32("primask at the end", primask());
    plat_exit(0);
}
