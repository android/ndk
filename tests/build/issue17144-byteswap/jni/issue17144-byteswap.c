#include <inttypes.h>
#include <sys/endian.h>

int16_t I16;
int32_t I32;
int64_t I64;

int16_t f16(int16_t i)
{
    return __swap16(i);
}

int32_t f32(int32_t i)
{
    return __swap32(i);
}

/* No need to check __swap64 because in all supported
   32-bit architectures it's implemented with two __swap32
int64_t f64(int64_t i)
{
    return __swap64(i);
}
*/

int main()
{
}
