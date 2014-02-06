
#include "RenderScript.h"

#include "ScriptC_mono.h"

using namespace android;
using namespace RSC;

int test_compute()
{
    bool failed = false;

    {
        sp<RS> rs = new RS();
        printf("New RS %p\n", rs.get());

        // only legitimate because this is a standalone executable
        bool r = rs->init("/system/bin");
        printf("Init returned %i\n", r);

        sp<const Element> e = Element::RGBA_8888(rs);
        printf("Element %p\n", e.get());

        Type::Builder tb(rs, e);
        tb.setX(128);
        tb.setY(128);
        sp<const Type> t = tb.create();
        printf("Type %p\n", t.get());


        sp<Allocation> a1 = Allocation::createSized(rs, e, 1000);
        printf("Allocation %p\n", a1.get());

        sp<Allocation> ain = Allocation::createTyped(rs, t);
        sp<Allocation> aout = Allocation::createTyped(rs, t);
        printf("Allocation %p %p\n", ain.get(), aout.get());

        sp<ScriptC_mono> sc = new ScriptC_mono(rs);
        printf("new script\n");

        sc->set_alloc(a1);
        sc->set_elem(e);
        sc->set_type(t);
        sc->set_script(sc);
        sc->set_script(NULL);
        sp<const Sampler> samp = Sampler::CLAMP_NEAREST(rs);
        sc->set_sampler(samp);

        // We read back the status from the script-side via a "failed" allocation.
        sp<const Element> failed_e = Element::BOOLEAN(rs);
        Type::Builder failed_tb(rs, failed_e);
        failed_tb.setX(1);
        sp<const Type> failed_t = failed_tb.create();
        sp<Allocation> failed_alloc = Allocation::createTyped(rs, failed_t);

        failed_alloc->copy1DRangeFrom(0, failed_t->getCount(), &failed);
        sc->bind_failed(failed_alloc);

        uint32_t *buf = new uint32_t[t->getCount()];
        for (uint32_t ct=0; ct < t->getCount(); ct++) {
            buf[ct] = ct | (ct << 16);
        }
        ain->copy1DRangeFrom(0, t->getCount(), buf);
        delete [] buf;

        sc->forEach_root(ain, aout);

        sc->invoke_foo(99, 3.1f);
        sc->set_g_f(39.9f);
        sc->set_g_i(-14);
        sc->invoke_foo(99, 3.1f);
        printf("for each done\n");

        sc->invoke_bar(47, -3, 'c', -7, 14, -8);

        // Verify a simple kernel.
        {
            static const uint32_t xDim = 7;
            static const uint32_t yDim = 7;
            sp<const Element> e = Element::I32(rs);
            Type::Builder tb(rs, e);
            tb.setX(xDim);
            tb.setY(yDim);
            sp<const Type> t = tb.create();
            sp<Allocation> kern1_in = Allocation::createTyped(rs, t);
            sp<Allocation> kern1_out = Allocation::createTyped(rs, t);

            int *buf = new int[t->getCount()];
            for (uint32_t ct=0; ct < t->getCount(); ct++) {
                buf[ct] = 5;
            }
            kern1_in->copy2DRangeFrom(0, 0, xDim, yDim, buf);
            delete [] buf;

            sc->forEach_kern1(kern1_in, kern1_out);
            sc->forEach_verify_kern1(kern1_out);

            rs->finish();
            failed_alloc->copy1DTo(&failed);
        }
    }

    return failed;
}

int main() {
    bool failed = test_compute();

    if (failed) {
        printf("TEST FAILED!\n");
    } else {
        printf("TEST PASSED!\n");
    }

    return failed;
}
