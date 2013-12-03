#include <stdio.h>
#include <arm_neon.h>

int fail_count = 0;

void dump_and_check(unsigned long long v, unsigned long long expected)
{
   printf("%016llx\n", v);
   fail_count += (v != expected);
}

static const uint8x8_t v00FF8 = { 0,0,0,0, 0xff,0xff,0xff,0xff }; // wrong
static const uint8x8_t vFF008 = { 0xff,0xff,0xff,0xff, 0,0,0,0 };

void test1()
{
    uint8x8_t v = v00FF8;
    dump_and_check(*(unsigned long long *)&v, 0xffffffff00000000ULL);
}

void test2()
{
    static const uint64_t local_Mask2 = { 0xffffffff00000000ULL };
    uint8x8_t v00FF8 = vcreate_u8(local_Mask2); //{ 0,0,0,0,0xff,0xff,0xff,0xff };
    dump_and_check(*(unsigned long long *)&v00FF8, 0xffffffff00000000ULL);
}

void test3()
{
    static const uint32_t vFF_ = { 0xFFFFFFFF };
    static const uint32_t v00_ = { 0x00000000 };
    static uint32x2_t v00FF_;
    v00FF_ = vset_lane_u32( vFF_, v00FF_, 1);
    v00FF_ = vset_lane_u32( v00_, v00FF_, 0);
    uint8x8_t v00FF8 = (uint8x8_t)v00FF_;
    dump_and_check(*(unsigned long long *)&v00FF8, 0xffffffff00000000ULL);
}

void test4()
{
    uint8x8_t mask_high = vcreate_u8(0xFFFFFFFF00000000ULL);
    uint8x8_t mask_low = vcreate_u8(0x00000000FFFFFFFFULL);
    uint8x8_t va = vdup_n_u8 ('a');
    uint8x8_t vb = vdup_n_u8 ('b');
    uint8x8_t in = vorr_u8(vand_u8 (va, mask_low), vand_u8 (vb, mask_high));
    dump_and_check(*(unsigned long long *)&in, 0x6262626261616161ULL);
}


int main()
{
    test1();
    test2();
    test3();
    test4();
    printf("fail_count = %d\n", fail_count);
    return (fail_count == 0)? 0 : 1;
}
