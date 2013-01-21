#ifndef LLVM_LIBCXX_SUPPORT_ANDROID_NL_TYPES_H
#define LLVM_LIBCXX_SUPPORT_ANDROID_NL_TYPES_H

#define NL_SETD 1
#define NL_CAT_LOCALE 1

typedef void* nl_catd;
typedef int nl_item;

nl_catd  catopen(const char*, int);
char*    catgets(nl_catd, int, int, const char*);
int      catclose(nl_catd);

#endif  /* LLVM_LIBCXX_SUPPORT_ANDROID_NL_TYPES_H */

