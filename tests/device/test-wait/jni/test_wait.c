/*
 * Copyright (C) 2008 The Android Open Source Project
 * All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 *  * Redistributions of source code must retain the above copyright
 *    notice, this list of conditions and the following disclaimer.
 *  * Redistributions in binary form must reproduce the above copyright
 *    notice, this list of conditions and the following disclaimer in
 *    the documentation and/or other materials provided with the
 *    distribution.
 *
 * THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS
 * "AS IS" AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT
 * LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS
 * FOR A PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE
 * COPYRIGHT OWNER OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT,
 * INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING,
 * BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS
 * OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED
 * AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY,
 * OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT
 * OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF
 * SUCH DAMAGE.
 */

#include <errno.h>
#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/resource.h>
#include <sys/wait.h>
#include <unistd.h>

#define CHILD_EXIT_CODE  111

typedef int (*wait_call_function)(pid_t child_pid);

static bool check_wait_call(const char *title, wait_call_function wait_func,
                            int expected_exit_code) {
  printf("Testing %s(): ", title);
  int cpid = fork();
  if (cpid < 0) {
    fprintf(stderr, "ERROR: fork() failed: %s\n", strerror(errno));
    return false;
  }

  if (cpid == 0) {  /* in the chid process */
    printf("Child created pid=%d parent_pid=%d\n", getpid(), getppid());
    exit(expected_exit_code);
  }

  /* in the parent process */
  printf("Parent waiting for child with pid=%d\n", cpid);
  int exit_code = wait_func(cpid);
  if (exit_code < 0)
    return false;

  if (exit_code != expected_exit_code) {
    fprintf(stderr, "ERROR: Child exited with code %d, expected %d\n",
            exit_code, expected_exit_code);
    return false;
  }
  printf("Testing %s(): OK\n", title);
  return true;
}

// To be called by check_wait_call() to check wait().
static int check_wait(pid_t child_pid) {
  int status = 0;
  pid_t ret = wait(&status);
  if (ret != child_pid) {
    fprintf(stderr, "ERROR: wait() returned %d, expected %d\n", ret, child_pid);
    return -1;
  }
  return WEXITSTATUS(status);
}

// To be called by check_wait_call() to check waitpid()
static int check_waitpid(pid_t child_pid) {
  int status = 0;
  pid_t ret = waitpid((pid_t)-1, &status, 0);
  if (ret != child_pid) {
    fprintf(stderr, "ERROR: waitpid() returned %d, expected %d\n", ret, child_pid);
    return -1;
  }
  return WEXITSTATUS(status);
}

// To be called by check_wait_call() to check wait3()
static int check_wait4(pid_t child_pid) {
  int status = 0;
  struct rusage ru;
  pid_t ret = wait4(-1, &status, 0, &ru);
  if (ret != child_pid) {
    fprintf(stderr, "ERROR: wait4() returned %d, expected %d\n", ret, child_pid);
    return -1;
  }
  return WEXITSTATUS(status);
}

int main(int argc, char *argv[]) {
  if (!check_wait_call("wait", check_wait, CHILD_EXIT_CODE + 0)) {
    return EXIT_FAILURE;
  }
  if (!check_wait_call("waitpid", check_waitpid, CHILD_EXIT_CODE + 1)) {
    return EXIT_FAILURE;
  }
  if (!check_wait_call("wait4", check_wait4, CHILD_EXIT_CODE + 3)) {
    return EXIT_FAILURE;
  }

  return EXIT_SUCCESS;
}

