#include <stddef.h>
#include <stdio.h>
#include <stdlib.h>
#include "unwind.h"

int main() {
#define CHECK_EQ(EXPECTED_VALUE, ACTUAL_VALUE) \
  do { \
    if ((int)(EXPECTED_VALUE) != (int)(ACTUAL_VALUE)) { \
      fprintf(stderr, "ASSERTION FAILURE: %s %d\n", __FILE__, __LINE__); \
      fprintf(stderr, "  expected value: %d (%s)\n", \
              (int)(EXPECTED_VALUE), #EXPECTED_VALUE); \
      fprintf(stderr, "  actual value: %d (%s)\n", \
              (int)(ACTUAL_VALUE), #ACTUAL_VALUE); \
      exit(EXIT_FAILURE); \
    } \
  } while (0)

#if defined(__arm__)
  CHECK_EQ(88, sizeof(struct _Unwind_Control_Block));
  CHECK_EQ(0, offsetof(struct _Unwind_Control_Block, exception_class));
  CHECK_EQ(8, offsetof(struct _Unwind_Control_Block, exception_cleanup));
#elif defined(__mips__)
  CHECK_EQ(24, sizeof(struct _Unwind_Exception));
  CHECK_EQ(0, offsetof(struct _Unwind_Exception, exception_class));
  CHECK_EQ(8, offsetof(struct _Unwind_Exception, exception_cleanup));
  CHECK_EQ(12, offsetof(struct _Unwind_Exception, private_1));
  CHECK_EQ(16, offsetof(struct _Unwind_Exception, private_2));
#elif defined(__i386__)
  CHECK_EQ(32, sizeof(struct _Unwind_Exception));
  CHECK_EQ(0, offsetof(struct _Unwind_Exception, exception_class));
  CHECK_EQ(8, offsetof(struct _Unwind_Exception, exception_cleanup));
  CHECK_EQ(12, offsetof(struct _Unwind_Exception, private_1));
  CHECK_EQ(16, offsetof(struct _Unwind_Exception, private_2));
#elif defined(__le32__)
  // TODO: What to check?
#else
#error "unsupported architecture"
#endif

  return EXIT_SUCCESS;
}
