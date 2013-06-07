#include "locale_impl.h"
#include <stdlib.h>

locale_t newlocale(int category_mask, const char* locale, locale_t base) {
    // Return LC_GLOBALE_LOCALE directly.
    if (base == LC_GLOBAL_LOCALE)
        return base;

    // Only accept "", "C" and "POSIX"
    if (*locale && strcmp(locale, "C") && strcmp(locale, "POSIX"))
        return LC_NULL_LOCALE;

    if (base == LC_NULL_LOCALE)
        base = calloc(1, sizeof(*base));

    return base;
}
