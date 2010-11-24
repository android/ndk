#include <cstdio>
#include <new>

int main()
{
    char *buf;
    try
    {
        buf = new char[512];
        throw "Memory allocation failure!";

        std::fprintf(stderr,"KO: Exception *not* raised!\n");
        return 1;
    }
    catch( char const* str )
    {
        std::printf("OK: Exception raised: %s\n", str);
    }
    return 0;
}
