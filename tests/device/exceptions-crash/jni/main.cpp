/* This test is meant to check that C++ exceptions do not crash
 * when running on Eclair or older platform releases. It will
 * always succeed on later versions of the  platform!
 */
#include <new>
#include <exception>
#include <cstdio>

static int foo(void)
{
    try {
	::printf("Hello ");
	throw std::exception();
    }
    catch (std::exception e) {
        ::printf(" World!\n");
    }
}

int main(int argc, char** argv)
{
    foo();
    return 0;
}
