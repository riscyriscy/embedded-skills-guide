struct P {
    int a;
    int b;
};
int f(P p)
{
    auto [x, y] = p;
    return x + y;
}
