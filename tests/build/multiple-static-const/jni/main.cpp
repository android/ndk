#include "main.h"

const int    Foo::CONST_1 = 42;
const double Foo::CONST_2 = 3.14159265;

extern void foo(void);

int main(void)
{
	foo();
	return 0;
}

