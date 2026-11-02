template <typename T> int size_of_t()
{
    if constexpr (sizeof(T) > 2) {
        return 4;
    } else {
        return 2;
    }
}
int a = size_of_t<char>();
