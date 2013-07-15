/*
 * Copyright (C) 2010 The Android Open Source Project
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *      http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */
#include <cpu-features.h>
#include <setjmp.h>
#include <stdio.h>
#include <signal.h>

#ifndef __arm__
#error "Only compile this file for an ARM target"
#endif

static int volatile g_signal_raised;
static sigjmp_buf g_jumper;

static void my_signal_handler(int signum) {
    g_signal_raised = 1;
    siglongjmp(g_jumper, 1);
}

static int32_t do_idiv(int32_t a, int32_t b) {
  __asm__ __volatile__(
      "sdiv %0, %0, %1\n\t"
      : "=&r"(a)
      : "r"(b)
      : "cc");
  return a;
}

// Check that the ARM idiv instruction is supported.
// Returns 1 on success, 0 otherwise.
static int check_idiv(void) {
    // Setup SIGILL signal handler.
    struct sigaction old_handler, new_handler;
    memset(&new_handler, 0, sizeof(new_handler));
    new_handler.sa_handler = my_signal_handler;
    sigaction(SIGILL, &new_handler, &old_handler);

    // Do the division.
    g_signal_raised = 0;
    if (sigsetjmp(g_jumper, 0) == 0)
        do_idiv(12345, 17);

    // Restore SIGILL handler.
    sigaction(SIGILL, &old_handler, NULL);

    return g_signal_raised == 0;
}

int main(void)
{
    uint32_t cpu_id = android_getCpuIdArm();
    printf("cpu-features reports the following CPUID: 0x%08x\n",
           cpu_id);

    uint64_t features = android_getCpuFeatures();
#ifdef __thumb2__
    const uint64_t idiv_flag = ANDROID_CPU_ARM_FEATURE_IDIV_THUMB2;
    const char* variant = "thumb";
#else
    const uint64_t idiv_flag = ANDROID_CPU_ARM_FEATURE_IDIV_ARM;
    const char* variant = "arm";
#endif

    printf("status of %s idiv instruction:\n", variant);
    int idiv_reported = (features & idiv_flag) != 0;
    printf("  reported      : %s\n",
           idiv_reported ? "supported" : "unsupported");

    int idiv_checked = check_idiv();
    printf("  runtime check : %s\n",
           idiv_checked ? "supported" : "unsupported");

    return (idiv_reported != idiv_checked);
}
