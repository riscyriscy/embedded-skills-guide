[[nodiscard]] int f(void);
[[maybe_unused]] static int x;
int g(int a)
{
    switch (a) {
    case 1:
        a++;
        [[fallthrough]];
    default:
        break;
    }
    return a;
}
