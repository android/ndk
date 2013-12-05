#include <algorithm>
#include <stdint.h>
#include <iostream>

double to_double(uint64_t x)
{
        union {double d; uint64_t x;} u;
        u.x = x;
        return u.d;
}

int main()
{
        std::vector<double> v = {
                to_double(4606672070890243784),
                to_double(4606672025854247510),
                to_double(4606671800674266141),
                to_double(4606671575494284772),
                to_double(4606672115926240057),
                to_double(4606672160962236330),
                to_double(4606672070890243784),
        };

        for (auto i : v)
                std::cout << i << std::endl;

        std::nth_element(v.begin(), v.begin() + 3, v.end());
        return 0;
}
