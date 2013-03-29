#include <stdio.h>
#include <stdlib.h>

#include "cpu-features.h"

#ifndef __arm__
#error "This source file should only be compiled on ARM!"
#endif

static void panic(const char* msg) {
  fprintf(stderr, "ERROR: %s\n", msg);
  exit(1);
}

int main(void) {
  int count, cpu_count = 10;
  uint64_t features, cpu_features = 0xaabdedf012934839ULL;
  uint32_t id, cpu_id = 0x436723ee;

  // Check that android_setCpuArm() can be called at program startup
  // and that android_getCpuCount() and android_getCpuFeatures()
  // will return the corresponding values.
  //
  printf("Setting cpu_count=%d, features=%08llx cpu_id=%08x\n",
         cpu_count, cpu_features, cpu_id);

  if (!android_setCpuArm(cpu_count, cpu_features, cpu_id))
    panic("Cannot call android_setCpu() at program startup!");

  count = android_getCpuCount();
  features = android_getCpuFeatures();
  id = android_getCpuIdArm();

  printf("Retrieved cpu_count=%d, features=%08llx cpu_id=%08x\n",
         count, features, id);

  if (count != cpu_count)
    panic("android_getCpuCount() didn't return expected value!");

  if (features != cpu_features)
    panic("android_getCpuFeatures() didn't return expected value!");

  if (id != cpu_id)
    panic("android_getCpuIdArm() didn't return expected value!");

  // Once one of the android_getXXX functions has been called,
  // android_setCpu() should always fail.
  if (android_setCpuArm(cpu_count, cpu_features, cpu_id))
    panic("android_setCpuArm() could be called twice!");

  printf("Second call to android_setCpu() failed as expected.\n");

  if (android_setCpu(cpu_count, cpu_features))
    panic("android_setCpu() could be called after android_setCpuArm()!");

  printf("Call to android_setCpu() failed as expected.\n");

  return 0;
}

