#include "RenderScript.h"
#include <sys/time.h>

#include "ScriptC_latency.h"

using namespace android;
using namespace RSC;

int main(int argc, char** argv)
{
    int iters = 100;
    int numElems = 1000;
    bool forceCpu = false;
    bool synchronous = false;

    if (argc >= 2) {
        iters = atoi(argv[1]);
        if (iters <= 0) {
            printf("iters must be positive\n");
            return 1;
        }
    }

    printf("iters = %d\n", iters);

    if (argc >= 3) {
        numElems = atoi(argv[2]);
        if (numElems <= 0) {
            printf("numElems must be positive\n");
            return 1;
        }
    }

    if (argc >= 4) {
        int temp = atoi(argv[3]);
        if (temp != 0)
            forceCpu = true;
    }

    if (argc >= 5) {
        int temp = atoi(argv[4]);
        if (temp != 0)
            synchronous = true;
    }

    if (forceCpu)
        printf("forcing CPU\n");

    if (synchronous)
        printf("forcing synchronous\n");

    printf("numElems = %d\n", numElems);

    sp<RS> rs = new RS();

    uint32_t flags = 0;
    if (forceCpu) flags |= RS_INIT_LOW_LATENCY;
    if (synchronous) flags |= RS_INIT_SYNCHRONOUS;

    bool r = rs->init("/system/bin", flags);

    sp<const Element> e = Element::U32(rs);

    Type::Builder tb(rs, e);
    tb.setX(numElems);
    sp<const Type> t = tb.create();

    uint32_t *buf = new uint32_t[numElems];

    sp<Allocation> ain = Allocation::createTyped(rs, t);
    sp<Allocation> aout = Allocation::createTyped(rs, t);

    sp<ScriptC_latency> sc = new ScriptC_latency(rs);

    struct timeval start, stop;

    gettimeofday(&start, NULL);

    for (int i = 0; i < iters; i++) {
        sc->forEach_root(ain, aout);
    }

    rs->finish();

    gettimeofday(&stop, NULL);

    long long elapsed = (stop.tv_sec * 1000000) - (start.tv_sec * 1000000) + (stop.tv_usec - start.tv_usec);
    printf("elapsed time : %lld microseconds\n", elapsed);
    printf("time per iter: %f microseconds\n", (double)elapsed / iters);

    gettimeofday(&start, NULL);

    for (int i = 0; i < iters; i++) {
        ain->copy1DFrom(buf);
        sc->forEach_root(ain, aout);
        aout->copy1DTo(buf);
    }

    rs->finish();

    gettimeofday(&stop, NULL);
    elapsed = (stop.tv_sec * 1000000) - (start.tv_sec * 1000000) + (stop.tv_usec - start.tv_usec);
    printf("elapsed time with copy : %lld microseconds\n", elapsed);
    printf("time per iter with copy: %f microseconds\n", (double)elapsed / iters);

    sc.clear();
    t.clear();
    e.clear();
    ain.clear();
    aout.clear();
}
