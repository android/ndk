#include <stdio.h>

#if defined(__arm__)
#include <arm_neon.h>
#define SP  "sp"
#elif defined(__i386__)
#include <xmmintrin.h>
#define SP  "esp"
typedef __m128 float32x4_t;
#elif defined(__mips__)
#define SP  "sp"
typedef float float32x4_t __attribute__ ((__vector_size__ (16)));
#elif !defined(__le32__)
#error unknown arch for type float32x4_t
#endif

#ifndef __le32__
class Vector4
{
  public:
    inline Vector4(float a, float b, float c, float d);
    inline Vector4() {}
    inline float32x4_t Set(float a, float b, float c, float d);
  private:
    float32x4_t m_floatVector;
} __attribute__((aligned(16)));

inline Vector4::Vector4(float a, float b, float c, float d)
{
    m_floatVector = Set(a, b, c, d);
}

inline float32x4_t Vector4::Set(float a, float b, float c, float d)
{
    float32x4_t value = { a, b, c, d };
    return value;
}

#if 1
Vector4 initVector4(float a, float b, float c, float d)
{
    return Vector4(a, b, c, d);
}
#else
void initVector4(Vector4 *v, float a, float b, float c, float d)
{
    v->Set(a, b, c, d);
}
#endif

float f;
Vector4 v;

int main()
{
    register int sp __asm(SP);
    printf("sp = %x\n", sp);
#if 1
    v = initVector4(f, f, f, f);
#else
    Vector4 v4;
    initVector4(&v4, f, f, f, f);
    v = v4;
#endif
    return 0;
}

#else // __le32__

int main()
{
    return 0; // Skip this test (Should not assume vector4 type on le32 triple)
}

#endif
