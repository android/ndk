#include <arm_neon.h>

namespace math {
    namespace internal {
#define _IOS_SHUFFLE_1032(vec) vrev64q_f32(vec)
#define _IOS_SHUFFLE_2301(vec) vcombine_f32(vget_high_f32(vec), vget_low_f32(vec))
        inline float32x4_t dot4VecResult(const float32x4_t& vec1, const float32x4_t& vec2) {
            float32x4_t result = vmulq_f32(vec1, vec2);
            result = vaddq_f32(result, _IOS_SHUFFLE_1032(result));
            result = vaddq_f32(result, _IOS_SHUFFLE_2301(result));
            return result;
        }

        inline float32x4_t fastRSqrt(const float32x4_t& vec) {
            float32x4_t result;
            result = vrsqrteq_f32(vec);
            result = vmulq_f32(vrsqrtsq_f32(vmulq_f32(result, result), vec), result);
            return result;
        }

    }
    typedef float32x4_t Vector3;

    inline Vector3 normalize(const Vector3& v1) {
        float32x4_t dot;
        dot = vsetq_lane_f32(0.0f, v1, 3);
        dot = internal::dot4VecResult(dot, dot);

        if (vgetq_lane_f32(dot, 0) == 0.0f) {
            return v1;
        } else {
            Vector3 result;
            result = vmulq_f32(v1, internal::fastRSqrt(dot));
            return result;
        }
    }

    inline Vector3 cross(const Vector3& v1, const Vector3& v2) {
        float32x4x2_t v_1203 = vzipq_f32(vcombine_f32(vrev64_f32(vget_low_f32(v1)), vrev64_f32(vget_low_f32(v2))), vcombine_f32(vget_high_f32(v1), vget_high_f32(v2)));
        float32x4x2_t v_2013 = vzipq_f32(vcombine_f32(vrev64_f32(vget_low_f32(v_1203.val[0])), vrev64_f32(vget_low_f32(v_1203.val[1]))), vcombine_f32(vget_high_f32(v_1203.val[0]), vget_high_f32(v_1203.val[1])));

        Vector3 result;
        result = vmlsq_f32(vmulq_f32(v_1203.val[0], v_2013.val[1]), v_1203.val[1], v_2013.val[0]);
        return result;
    }
}

void _f_with_internal_compiler_error_in_reload_cse_simplify_operands(const math::Vector3& v1, const math::Vector3& v2) {
    math::normalize(math::cross(v1, v2));
}
