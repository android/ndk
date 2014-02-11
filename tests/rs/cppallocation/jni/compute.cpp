
#include "RenderScript.h"

#include "ScriptC_multiply.h"

using namespace android;
using namespace RSC;

int main(int argc, char** argv)
{

    uint32_t numElems = 1024;

    if (argc >= 2) {
        int tempNumElems = atoi(argv[1]);
        if (tempNumElems < 1) {
            printf("numElems must be greater than 0\n");
            return 1;
        }
        numElems = (uint32_t) tempNumElems;
    }

    sp<RS> rs = new RS();

    bool r = rs->init("/system/bin");

    sp<const Element> e = Element::U32(rs);

    Type::Builder tb(rs, e);
    tb.setX(numElems);
    sp<const Type> t = tb.create();

    sp<Allocation> ain = Allocation::createTyped(rs, t);
    sp<Allocation> aout = Allocation::createTyped(rs, t);

    sp<ScriptC_multiply> sc = new ScriptC_multiply(rs);

    uint32_t* buf = new uint32_t[numElems];
    for (uint32_t ct=0; ct < numElems; ct++) {
        buf[ct] = (uint32_t)ct;
    }

    ain->copy1DRangeFrom(0, numElems, buf);

    sc->forEach_multiply(ain, aout);

    aout->copy1DRangeTo(0, numElems, buf);

    for (uint32_t ct=0; ct < numElems; ct++) {
        if (buf[ct] !=  ct * 2) {
            printf("Mismatch at location %d: %u\n", ct, buf[ct]);
            return 1;
        }
    }

    printf("Test successful with %u elems!\n", numElems);

    sc.clear();
    t.clear();
    e.clear();
    ain.clear();
    aout.clear();
}
