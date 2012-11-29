#include <arm_neon.h>

struct Matrix43 {
    float32x4_t row0;
    float32x4_t row1;
    float32x4_t row2;
    float32x4_t row3;
};

__attribute__((always_inline)) inline Matrix43 operator*(const Matrix43& m1, const Matrix43& m2) {
    Matrix43 rr;
    rr.row0 = vmulq_n_f32(         m2.row0, vgetq_lane_f32(m1.row0, 0));
    rr.row0 = vmlaq_n_f32(rr.row0, m2.row1, vgetq_lane_f32(m1.row0, 1));
    rr.row0 = vmlaq_n_f32(rr.row0, m2.row2, vgetq_lane_f32(m1.row0, 2));

    rr.row1 = vmulq_n_f32(         m2.row0, vgetq_lane_f32(m1.row1, 0));
    rr.row1 = vmlaq_n_f32(rr.row1, m2.row1, vgetq_lane_f32(m1.row1, 1));
    rr.row1 = vmlaq_n_f32(rr.row1, m2.row2, vgetq_lane_f32(m1.row1, 2));

    rr.row2 = vmulq_n_f32(         m2.row0, vgetq_lane_f32(m1.row2, 0));
    rr.row2 = vmlaq_n_f32(rr.row2, m2.row1, vgetq_lane_f32(m1.row2, 1));
    rr.row2 = vmlaq_n_f32(rr.row2, m2.row2, vgetq_lane_f32(m1.row2, 2));

    rr.row3 = vmlaq_n_f32(m2.row3, m2.row0, vgetq_lane_f32(m1.row3, 0));
    rr.row3 = vmlaq_n_f32(rr.row3, m2.row1, vgetq_lane_f32(m1.row3, 1));
    rr.row3 = vmlaq_n_f32(rr.row3, m2.row2, vgetq_lane_f32(m1.row3, 2));
    return rr;
}

void _f_with_internal_compiler_error(const Matrix43& m, const void* a1, const void* a2) {
    m * m * m;
}
