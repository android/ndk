#include <new>
#include <exception>
#include <cstdio>

int foo(void)
{
    try {
        ::printf("Hello ");
        throw std::exception();
    }
    catch (std::exception e) {
        ::printf(" World!\n");
    }
}

