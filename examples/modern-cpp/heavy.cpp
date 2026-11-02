#include <string>

#include "print.h"

/* A program that looks small. std::string allocates, and its error paths throw. */
int main()
{
    std::string name = "sensor";
    name += "-";
    name += std::to_string(7);
    print_u32("length", name.size());
    plat_exit(0);
}
