#include <iostream>
#include <chrono>
#include <ratio>

#if !(__GNUC__ > 4 || (__GNUC__ == 4 && __GNUC_MINOR__ >= 7))
// In GCC 4.6 and below, "steady_clock" is called "monotonic_clock",
// and "is_steady" is called "is_monotonic"
namespace std {
namespace chrono {
    typedef monotonic_clock steady_clock;
}
}
#define is_steady is_monotonic
#endif

template <typename C>
void printClockData (bool &is_high_res, bool &is_steady)
{
    using namespace std;

    cout << "- precision: ";
    // if time unit is less or equal one millisecond
    typedef typename C::period P;// type of time unit
    if (ratio_less_equal<P,milli>::value) {
       // convert to and print as milliseconds
       typedef typename ratio_multiply<P,kilo>::type TT;
       cout << fixed << double(TT::num)/TT::den
            << " milliseconds" << endl;
       is_high_res = true;
    }
    else {
        // print as seconds
        cout << fixed << double(P::num)/P::den << " seconds" << endl;
       is_high_res = false;
    }
    cout << "- is_steady: " << boolalpha << C::is_steady << endl;
    is_steady = C::is_steady;
}

int main()
{
    bool is_high_res1, is_high_res2, is_high_res3, is_steady;
    std::cout << "system_clock: " << std::endl;
    printClockData<std::chrono::system_clock>(is_high_res1, is_steady);
    std::cout << "\nhigh_resolution_clock: " << std::endl;
    printClockData<std::chrono::high_resolution_clock>(is_high_res2, is_steady);
    std::cout << "\nsteady_clock: " << std::endl;
    printClockData<std::chrono::steady_clock>(is_high_res3, is_steady);

    return (is_high_res1 && is_high_res2 && is_high_res3 && is_steady)? 0 : 1;
}
