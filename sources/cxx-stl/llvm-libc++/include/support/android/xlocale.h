#ifndef LLVM_LIBCXX_SUPPORT_ANDROID_XLOCALE_H
#define LLVM_LIBCXX_SUPPORT_ANDROID_XLOCALE_H

extern "C" {

typedef struct locale_struct*  locale_t;

struct locale_struct {
    void* dummy;
};

}  // extern "C"

#endif  // LLVM_LIBCXX_SUPPORT_ANDROID_XLOCALE_H
