#include <ostream>
#include <sstream>
#include <string>

#define BOOST_TEST_NO_MAIN
#define BOOST_TEST_ALTERNATIVE_INIT_API
#include <boost/test/included/unit_test.hpp>

static bool my_init_function()
{
    return true;
}

void X()
{
     char *apArgVals[] =
        {
                "AndroidSDKTest",
                 "--log_level=warning",
                 "--show_progress=yes",
                0
        };
    int nArgc = 3;
    ::boost::unit_test::unit_test_main(::my_init_function, nArgc, apArgVals);
}

int main(int argc, char *argv[])
{
#ifdef CALL_X
    X();
#else
    ::boost::unit_test::unit_test_main(::my_init_function, argc, argv);
#endif
    return 0;
}

