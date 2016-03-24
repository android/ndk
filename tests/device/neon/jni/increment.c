#include <stdio.h>
#include <arm_neon.h>

void neon_inc (uint8x16_t* value) {
    uint8x16_t op2 = vmovq_n_u8 (1);
    *value = vaddq_u8 (*value, op2);
}

int main () {
    uint8_t bytes[] = {1, 2, 3, 4, 5, 6, 7, 8, 9, 10, 11, 12, 13, 14, 15, 16};
    uint8_t values[16];
    int status = 0, i;
    uint8x16_t vector = vld1q_u8 (bytes);
    neon_inc (&vector);
    vst1q_u8 (values, vector);
    for (i = 0; i < 16; ++i)
        if (values[i] != i + 2) {
            printf ("Fail for i = %2d, actual value: %2d, expected value: %2d\n", i, values[i], i + 2);
            ++status;
        }

    if (status == 0)
        printf ("ok\n");
    return status;
}

