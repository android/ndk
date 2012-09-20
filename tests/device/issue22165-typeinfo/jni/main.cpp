#include <iostream>
#include <stdexcept>

#include "myexception.h"
#include "throwable.h"

int main(int /*argc*/, char** /*argv*/)
{
    int result = 0;
    std::cout << "call throw_an_exception()" << std::endl;
    
    try {
        throw_an_exception();
    } catch (my_exception const& e) {
        std::cout << "my_exception caught!" << std::endl;
    } catch (std::exception const& e) {
        std::cout << "ERROR: exception caught!" << std::endl;
        result = 1;
    }

    std::cout << "finished" << std::endl;
    
    return result;
}
