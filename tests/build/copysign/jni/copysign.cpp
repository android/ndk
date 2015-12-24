#include <cmath>

template <typename T>
T foo(T a) {
    return std::copysign(a, static_cast<T>(-1.0));
}
