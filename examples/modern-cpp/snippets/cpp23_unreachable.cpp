#include <utility>
int f(int a)
{
    if (a) {
        return 1;
    }
    std::unreachable();
}
