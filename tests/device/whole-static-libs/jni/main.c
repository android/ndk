/* A small program used to check that LOCAL_WHOLE_STATIC_LIBRARIES builds
 * and works properly. Here, we check that the 'foo2' function which is
 * never called from main is still included in the final executable
 * image.
 */
#include <stdio.h>
#include <unistd.h>
#include <dlfcn.h>

int main(int argc, char *argv[])
{
    char cwd[PATH_MAX];
    if (getcwd(cwd, sizeof(cwd)) == NULL) {
        perror("getcwd");
        return 1;
    }

    char buf[PATH_MAX];
    sprintf(buf, "%s/libbar.so", cwd);

    void* lib = dlopen(buf, RTLD_NOW);
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
