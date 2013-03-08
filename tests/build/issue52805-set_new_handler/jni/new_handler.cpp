#include <new>
std::new_handler get_and_reset_new_handler()
{
    return std::set_new_handler(0);
}
