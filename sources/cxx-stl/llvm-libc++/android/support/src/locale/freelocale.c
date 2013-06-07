#include "locale_impl.h"
#include <stdlib.h>

void freelocale(locale_t loc) {
    if (loc != LC_NULL_LOCALE && loc != LC_GLOBAL_LOCALE)
        free(loc);
}
