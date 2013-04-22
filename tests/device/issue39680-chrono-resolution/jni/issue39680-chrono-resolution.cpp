#include <iostream>
#include <chrono>
#include <ratio>

// In NDK GCC 4.6 and below, "steady_clock" is called "monotonic_clock",
// and "is_steady" is called "is_monotonic".  One may be tempted to use
// __GLIBCXX__ to detect it by doing
//
//   # if __GLIBCXX__ < 20120920  /* 20120920 is the date of gcc-4.7 in NDK */
//
// But __GLIBCXX__ get date from gcc/DATESTAMP.  Although it never changes
// (so far) once deposit in toolchain/gcc.git, it gets daily bump in upstream.
// Thus, this approach may not work in other gcc release.
//
// We can detect it by
//
//     #if !(__GNUC__ > 4 || (__GNUC__ == 4 && __GNUC_MINOR__ >= 7))
//
// But unfortunately clang uses gcc libstdc++ w/o defining __GNUC__ at all.
// Since clang now sides with gcc-4.7, we need the following intead
//
//     #if !(__GNUC__ > 4 || (__GNUC__ == 4 && __GNUC_MINOR__ >= 7) || defined(__clang__))
//
// This approach won't be valid if clang sides with gcc4.6 (in standalone mode, for
// example).
//
// ToDo: better approach

#if !(__GNUC__ > 4 || (__GNUC__ == 4 && __GNUC_MINOR__ >= 7) || defined(__clang__))
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
