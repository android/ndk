#include <errno.h>
#include <locale.h>
#include <stdlib.h>
#include "locale_impl.h"

locale_t duplocale(locale_t loc) {
    if (loc == LC_GLOBAL_LOCALE)
        return loc;
    if (loc == LC_NULL_LOCALE) {
        errno = EINVAL;
        return LC_NULL_LOCALE;
    }
    locale_t copy = calloc(1, sizeof(*loc));
    copy[0] = loc[0];
    return copy;
}
