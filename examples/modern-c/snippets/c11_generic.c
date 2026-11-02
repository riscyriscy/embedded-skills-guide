#define KIND(x) _Generic((x), int: 1, default: 0)
int a = KIND(1);
