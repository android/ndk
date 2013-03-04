// Contains an implementation of nl_types functions missing from bionic.
//
// TODO(ajwong): Make these work.

#include <nl_types.h>

nl_catd catopen(const char* name, int oflag) {
#warning Not implemented
  return 0;
}

char* catgets(nl_catd catd, int set_id, int msg_id, const char* s) {
#warning Not implemented
  return 0;
}

int catclose(nl_catd catd) {
#warning Not implemented
  return 0;
}
