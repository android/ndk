#include <iostream>

int main()
{
#if !defined(_GLIBCXX_HAS_GTHREADS)
#error _GLIBCXX_HAS_GTHREADS is not defined
#endif
}
