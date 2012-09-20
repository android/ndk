#ifndef MYEXCEPTION_H__
#define MYEXCEPTION_H__

#include <string>
#include <stdexcept>
#include <iostream>

class my_exception : public std::runtime_error
{
public:
    my_exception(std::string const& w)
        : std::runtime_error(w)
    { 
        std::cout << "yep! i am in constructor" << std::endl; 
    }

    virtual ~my_exception() throw() 
    {
        std::cout << "yep! i am in desctuctor" << std::endl; 
    }
};

#endif //MYEXCEPTION_H__
