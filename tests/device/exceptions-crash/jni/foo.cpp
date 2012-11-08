#include <new>
#include <exception>
#include <cstdio>

void foo()
{
    try {
        ::printf("Hello ");
        throw std::exception();
    }
    catch (std::exception e) {
        ::printf(" World!\n");
    }
}

