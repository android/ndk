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
int printClockData ()
{
    using namespace std;
    bool high_res_and_steady = C::is_steady;

    cout << "- precision: ";
    // if time unit is less or equal one millisecond
    typedef typename C::period P;// type of time unit
    if (ratio_less_equal<P,milli>::value) {
       // convert to and print as milliseconds
       typedef typename ratio_multiply<P,kilo>::type TT;
       cout << fixed << double(TT::num)/TT::den
            << " milliseconds" << endl;
       high_res_and_steady &= true;
    }
    else {
        // print as seconds
        cout << fixed << double(P::num)/P::den << " seconds" << endl;
       high_res_and_steady = false;
    }
    cout << "- is_steady: " << boolalpha << C::is_steady << endl;

    return high_res_and_steady;
}

int main()
{
    std::cout << "system_clock: " << std::endl;
    bool res1 = printClockData<std::chrono::system_clock>();
    std::cout << "\nhigh_resolution_clock: " << std::endl;
    bool res2 = printClockData<std::chrono::high_resolution_clock>();
    std::cout << "\nsteady_clock: " << std::endl;
    bool res3 = printClockData<std::chrono::steady_clock>();
 
    return (res1 && res2 && res3)? 0 : 1;
}
