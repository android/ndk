#include <cerrno>
#include <cstddef>
#include <cstdio>
#include <sstream>

int main(void)
{
    const char* inputTest = "0";

    double f = 6;
    std::istringstream ss(inputTest);
    printf ("Doing the read\n");
    ss >> f; //Crash in 4.7, fine in 4.6
    printf ("Here it is %f\n", f);
}
