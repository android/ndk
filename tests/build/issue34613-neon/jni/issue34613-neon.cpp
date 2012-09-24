#include <vector>
#include <arm_neon.h>

void x(float32x4_t v0, float32_t f)
{
  float32x4_t vv = vmulq_n_f32( v0, f);
}

int main()
{
   std::vector<float32_t> my_vector;
   return 0;
}

