#include <ostream>
#include <netinet/in.h>

class ClassA : public std::basic_streambuf<char, std::char_traits<char> >, public std::ostream
{
   public:
    ClassA(const uint32_t foo);
};

ClassA::ClassA(const uint32_t foo) : std::ostream(this)
{
    uint32_t be = htonl(foo);
}
