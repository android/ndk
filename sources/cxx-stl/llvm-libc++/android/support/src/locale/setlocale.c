#include <errno.h>
#include "locale_impl.h"

char *setlocale(int category, const char *locale) {
    // Sanity check.
    if (locale == NULL) {
        errno = EINVAL;
        return NULL;
    }
    // Only accept "", "C" or "POSIX", all equivalent on Android.
    if (*locale && strcmp(locale, "C") && strcmp(locale, "POSIX")) {
        errno = EINVAL;
        return NULL;
    }
    // The function returns a char* but the caller is not supposed to
    // modify it. Just to a type cast. If the caller tries to write to
    // it, it will simply segfault.
    static const char C_LOCALE_SETTING[] = "C";
    return (char*) C_LOCALE_SETTING;
}
