// Contains an implementation of stdlib functions missing from bionic.
//
// TODO(ajwong): Make these work.

#include <wctype.h>

int iswblank(wint_t c) {
  return 0;
}
