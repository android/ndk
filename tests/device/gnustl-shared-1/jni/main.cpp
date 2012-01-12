#include <cstdio>
#include <exception>

int foo(void);

int main(void)
{
    try {
        (void) foo();
    }
    // Catch all exceptions. Note that if we used catch (std::exception& e)
    // instead, we would need to activate rtti as well to avoid crashing
    // in the C++ runtime.
    catch (...) {
        ::printf(" World!\n");
    }
    return 0;
}