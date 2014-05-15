typedef unsigned int uint32_t;

extern const uint32_t gMask_00FF00FF;

uint32_t SkAlphaMulQ(uint32_t c, unsigned scale) {
    uint32_t mask = gMask_00FF00FF;

    uint32_t rb = ((c & mask) * scale) >> 8;
    uint32_t ag = ((c >> 8) & mask) * scale;
    return (rb & mask) | (ag & ~mask);
}

int main()
{
}
