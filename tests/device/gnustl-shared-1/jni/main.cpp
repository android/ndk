#include <cstdio>
#include <exception>

int foo(void);

int main(void)
{
    try {
        (void) foo();
    }
    catch (std::exception e) {
        ::printf(" World!\n");
    }
    return 0;
}