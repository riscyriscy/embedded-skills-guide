#include <utility>
enum class E : unsigned char { A, B };
unsigned char f(E e)
{
    return std::to_underlying(e);
}
