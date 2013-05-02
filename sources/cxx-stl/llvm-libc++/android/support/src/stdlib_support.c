// Contains an implementation of all stdlib functions missing from bionic.
//
// TODO(ajwong): Make these work.

#include <stdlib.h>

long double strtold(const char* nptr, char** endptr) {
#warning Not implemented
  return 0;
}

int mbtowc(wchar_t *pwc, const char *pmb, size_t max) {
#warning Not implemented
  return 0;
}
