#include <emmintrin.h>

int main()
{
    __m64 *p;
    __m128 *q;
    int p_isaligned, q_isaligned;
    p = _mm_malloc(7*sizeof(*p),sizeof(*p));
    q = _mm_malloc(5*sizeof(*q),sizeof(*q));
    p_isaligned = ((int)p % sizeof(*p)) == 0;
    q_isaligned = ((int)q % sizeof(*q)) == 0;
    free(p);
    free(q);
    return (p_isaligned && q_isaligned)? 0 : 1;
}
