#include <new>
#include <exception>
#include <cstdio>

int foo(void)
{
    ::printf("Hello ");
    throw std::exception();
}

