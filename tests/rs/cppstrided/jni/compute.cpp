
#include "RenderScript.h"

#include "ScriptC_multiply.h"

using namespace android;
using namespace RSC;

int main(int argc, char** argv)
{

    uint32_t numElems = 1024;
    uint32_t stride = 1025;

    if (argc >= 2) {
        int tempStride = atoi(argv[1]);
        if (tempStride < 1024) {
            printf("stride must be greater than or equal to 1024\n");
            return 1;
        }
        stride = (uint32_t) tempStride;
    }

    sp<RS> rs = new RS();

    bool r = rs->init("/system/bin");

    sp<const Element> e = Element::U32(rs);

    Type::Builder tb(rs, e);
    tb.setX(numElems);
    tb.setY(numElems);
    sp<const Type> t = tb.create();

    sp<Allocation> ain = Allocation::createTyped(rs, t);
    sp<Allocation> aout = Allocation::createTyped(rs, t);

    sp<ScriptC_multiply> sc = new ScriptC_multiply(rs);

    uint32_t* buf = (uint32_t*) malloc(stride * numElems * sizeof(uint32_t));
    if (!buf) {
        printf("malloc failed\n");
        return 1;
    }

    for (uint32_t i = 0; i < numElems; i++) {
        for (uint32_t ct=0; ct < numElems; ct++) {
            *(buf+(stride*i)+ct) = (uint32_t)ct + (i * numElems);
        }
    }

    ain->copy2DStridedFrom(buf, stride * sizeof(uint32_t));

    sc->forEach_multiply(ain, aout);

    aout->copy2DStridedTo(buf, stride * sizeof(uint32_t));

    for (uint32_t i = 0; i < numElems; i++) {
        for (uint32_t ct=0; ct < numElems; ct++) {
            if (*(buf+(stride*i)+ct) != (uint32_t)(ct + (i * numElems)) * 2) {
                printf("Mismatch at location %d, %d: %u\n", i, ct, *(buf+(stride*i)+ct));
                return 1;
            }
        }
    }

    printf("Test successful with %u stride!\n", stride);

    sc.clear();
    t.clear();
    e.clear();
    ain.clear();
    aout.clear();
}
