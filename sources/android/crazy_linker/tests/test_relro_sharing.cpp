// Copyright (c) 2013 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

// A crazy linker test to:
// - Load a library (libfoo.so) with the linker.
// - Find the address of the "Foo" function in it.
// - Call the function.
// - Close the library.

#include <errno.h>
#include <stdarg.h>
#include <stdio.h>
#include <stdlib.h>
#include <sys/socket.h>
#include <sys/uio.h>
#include <sys/wait.h>
#include <unistd.h>

#include <crazy_linker.h>

#include "test_util.h"

typedef void (*FunctionPtr)();

int main() {

  crazy_context_t* context = crazy_context_create();
  crazy_library_t* library;

  // Load at fixed address to simplify testing.
  crazy_context_set_load_address(context, 0x20000000);

  // Load libfoo_with_relro.so
  if (!crazy_library_open(&library, "libfoo_with_relro.so", context)) {
    Panic("Could not open library: %s\n", crazy_context_get_error(context));
  }

  printf("Library loaded\n");

  int pipes[2];
  if (socketpair(AF_UNIX, SOCK_STREAM, 0, pipes) < 0)
    Panic("Could not create socket pair: %s", strerror(errno));

  pid_t child = fork();
  if (child < 0)
    Panic("Could not fork test program!");

  if (child == 0) {
    // In the child.
    crazy_library_info_t info;

    printf("Child waiting for relro fd\n");
    // Receive relro information from parent.
    int relro_fd = -1;
    if (ReceiveFd(pipes[0], &relro_fd) < 0)
      Panic("Could not receive relro descriptor from parent");

    printf("Child received relro fd %d\n", relro_fd);

    int ret = TEMP_FAILURE_RETRY(::read(pipes[0], &info, sizeof(info)));
    if (ret != static_cast<int>(sizeof(info)))
      Panic("Could not receive relro information from parent");

    info.relro_fd = relro_fd;
    printf("Child received relro load=%p start=%p size=%p\n",
           (void*)info.load_address,
           (void*)info.relro_start,
           (void*)info.relro_size);

    if (!crazy_library_use_relro_sharing(library,
                                         info.relro_start,
                                         info.relro_size,
                                         info.relro_fd,
                                         context)) {
      Panic("Could not use RELRO sharing: %s",
            crazy_context_get_error(context));
    }

    printf("RELRO used in child process\n");

    CheckRelroMaps(1);

    FunctionPtr foo_func;
    if (!crazy_library_find_symbol(
             library, "Foo", reinterpret_cast<void**>(&foo_func)))
      Panic("Could not find 'Foo' in library");

    printf("Calling Foo()\n");
    (*foo_func)();

    printf("Foo called, exiting\n");

    exit(0);

  } else {
    // In the parent.
    crazy_library_info_t info;

    printf("Parent enabling RELRO sharing\n");

    // Enable RELRO sharing.
    if (!crazy_library_enable_relro_sharing(library, context))
      Panic("Could not enable RELRO sharing: %s",
            crazy_context_get_error(context));

    if (!crazy_library_get_info(library, context, &info))
      Panic("Could not get library info: %s", crazy_context_get_error(context));

    printf(
        "Parent relro info load_addr=%p load_size=%p relro_start=%p "
        "relro_size=%p relro_fd=%d\n",
        (void*)info.load_address,
        (void*)info.load_size,
        (void*)info.relro_start,
        (void*)info.relro_size,
        info.relro_fd);

    CheckRelroMaps(1);

    if (SendFd(pipes[1], info.relro_fd) < 0)
      Panic("Parent could not send RELRO fd: %s", strerror(errno));

    int ret = TEMP_FAILURE_RETRY(::write(pipes[1], &info, sizeof(info)));
    if (ret != static_cast<int>(sizeof(info)))
      Panic("Parent could not send RELRO info: %s", strerror(errno));

    printf("Parent waiting for child\n");

    // Wait for child to complete.
    int status;
    waitpid(child, &status, 0);

    if (WIFSIGNALED(status))
      Panic("Child terminated by signal!!\n");
    else if (WIFEXITED(status)) {
      int child_status = WEXITSTATUS(status);
      if (child_status != 0)
        Panic("Child terminated with status=%d\n", child_status);
    } else
      Panic("Child exited for unknown reason!!\n");
  }

  printf("Closing library\n");
  crazy_library_close(library);

  crazy_context_destroy(context);
  return 0;
}
