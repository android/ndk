#include <iostream>

#include "myexception.h"
#include "throwable.h"

int throw_an_exception() 
{
    std::cout << "throw_an_exception()" << std::endl;
    throw my_exception("my exception");
}