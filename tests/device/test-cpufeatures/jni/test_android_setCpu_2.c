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

  count = android_getCpuCount();
  features = android_getCpuFeatures();

  printf("Retrieved cpu_count=%d, features=%08llx\n",
         count, features);

  // Check that android_setCpu() can be called at program startup
  // and that android_getCpuCount() and android_getCpuFeatures()
  // will return the corresponding values.
  //
  printf("Trying to set cpu_count=%d, features=%08llx\n",
         cpu_count,
         cpu_features);

  if (android_setCpu(cpu_count, cpu_features))
    panic("android_setCpu() call should have failed!");

  printf("android_setCpu() call fail as expected.\n");
  return 0;
}
