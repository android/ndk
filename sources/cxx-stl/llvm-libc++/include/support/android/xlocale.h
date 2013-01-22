#ifndef LLVM_LIBCXX_SUPPORT_ANDROID_XLOCALE_H
#define LLVM_LIBCXX_SUPPORT_ANDROID_XLOCALE_H

#ifdef __cplusplus
extern "C" {
#endif

typedef struct locale_struct*  locale_t;

struct locale_struct {
    void* dummy;
};

#ifdef __cplusplus
}  // extern "C"
#endif

#endif  // LLVM_LIBCXX_SUPPORT_ANDROID_XLOCALE_H
