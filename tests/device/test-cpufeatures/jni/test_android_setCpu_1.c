#include <stdio.h>
#include <stdlib.h>

#include "cpu-features.h"

static void panic(const char* msg) {
  fprintf(stderr, "ERROR: %s\n", msg);
  exit(1);
}

int main(void) {
  int count, cpu_count = 10;
  uint64_t features, cpu_features = 0xaabdedf012934839ULL;

  // Check that android_setCpu() can be called at program startup
  // and that android_getCpuCount() and android_getCpuFeatures()
  // will return the corresponding values.
  //
  printf("Setting cpu_count=%d, features=%08llx\n",
         cpu_count,
         cpu_features);
  if (!android_setCpu(cpu_count, cpu_features))
    panic("Cannot call android_setCpu() at program startup!");

  count = android_getCpuCount();
  features = android_getCpuFeatures();

  printf("Retrieved cpu_count=%d, features=%08llx\n",
         count, features);

  if (count != cpu_count)
    panic("android_getCpuCount() didn't return expected value!");

  if (features != cpu_features)
    panic("android_getCpuFeatures() didn't return expected value!");

  // Once one of the android_getXXX functions has been called,
  // android_setCpu() should always fail.
  if (android_setCpu(cpu_count, cpu_features))
    panic("android_setCpu() could be called twice!");

  printf("Second call to android_setCpu() failed as expected.\n");
  return 0;
}

