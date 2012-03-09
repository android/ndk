/* A small program used to check that LOCAL_WHOLE_STATIC_LIBRARIES builds
 * and works properly. Here, we check that the 'foo2' function which is
 * never called from main is still included in the final executable
 * image.
 */
#include <stdio.h>
#include <dlfcn.h>

int main(void)
{
    void*  lib;

    lib = dlopen("/data/local/tmp/ndk-tests/libbar.so", RTLD_NOW);
    if (lib == NULL) {
        fprintf(stderr, "Could not dlopen(\"libbar.so\"): %s\n", dlerror());
        return 1;
    }
    if (dlsym(lib, "foo2") == NULL) {
        fprintf(stderr, "Symbol 'foo2' is missing from shared library!!\n");
        return 2;
    }
    dlclose(lib);
    return 0;
}
