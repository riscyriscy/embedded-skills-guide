template <typename T>
concept SmallInt = sizeof(T) <= 2;
template <SmallInt T> T twice(T v)
{
    return static_cast<T>(v * 2);
}
short s = twice<short>(1);
