constexpr int f(int v)
{
    if consteval {
        return v;
    } else {
        return v + 1;
    }
}
