#include <span>
int sum(std::span<const int> s)
{
    int t = 0;
    for (int v : s)
        t += v;
    return t;
}
