#include <android/api-level.h>
#include <dlfcn.h>

#include <minitest/minitest.h>

#if __ANDROID_API__ >= 8
Dl_info info;
extern "C" int my_dladdr(const void* addr, Dl_info *info);
#else
void *info;
extern "C" int my_dladdr(const void* addr, void **info);
#endif

TEST(libdl, my_dladdr) {
    void *h, *libdl = dlopen("libc.so", RTLD_NOW);
    EXPECT_NE(0, libdl);
    h = dlsym(libdl, "printf");
    EXPECT_NE(0, h);
    int r = my_dladdr((char*)h+1, &info);
#if __ANDROID_API__ >= 8
    EXPECT_NE(0, r);
    EXPECT_EQ(h, info.dli_saddr);
    EXPECT_EQ(0, strcmp("libc.so", info.dli_fname));
#else
    EXPECT_EQ(0, r);
#endif
}
