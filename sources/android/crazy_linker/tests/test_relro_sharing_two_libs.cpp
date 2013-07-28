// Copyright (c) 2013 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

// Same as test_relro_sharing.cpp, but uses two libraries at the same
// time (libfoo_with_relro.so and libbar_with_relro.so), each one of
// them gets its own shared RELRO.

#include <errno.h>
#include <pthread.h>
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

struct Library {
  const char* name;
  crazy_library_t* library;
  crazy_library_info_t info;

  void Init(const char* name, crazy_context_t* context) {
    printf("Loading %s\n", name);
    this->name = name;
    if (!crazy_library_open(&this->library, name, context)) {
      Panic("Could not open %s: %s\n", name, crazy_context_get_error(context));
    }
  }

  void Close() { crazy_library_close(this->library); }

  void EnableSharedRelro(crazy_context_t* context) {
    if (!crazy_library_enable_relro_sharing(this->library, context)) {
      Panic("Could not enable %s RELRO sharing: %s",
            this->name,
            crazy_context_get_error(context));
    }

    if (!crazy_library_get_info(this->library, context, &this->info))
      Panic("Could not get %s library info: %s",
            this->name,
            crazy_context_get_error(context));

    printf(
        "Parent %s relro info load_addr=%p load_size=%p"
        " relro_start=%p relro_size=%p relro_fd=%d\n",
        this->name,
        (void*)this->info.load_address,
        (void*)this->info.load_size,
        (void*)this->info.relro_start,
        (void*)this->info.relro_size,
        this->info.relro_fd);
  }

  void SendRelroInfo(int fd) {
    if (SendFd(fd, this->info.relro_fd) < 0) {
      Panic("Could not send %s RELRO fd: %s", this->name, strerror(errno));
    }

    int ret = TEMP_FAILURE_RETRY(::write(fd, &this->info, sizeof(this->info)));
    if (ret != static_cast<int>(sizeof(this->info))) {
      Panic("Parent could not send %s RELRO info: %s",
            this->name,
            strerror(errno));
    }
  }

  void ReceiveRelroInfo(int fd) {
    // Receive relro information from parent.
    int relro_fd = -1;
    if (ReceiveFd(fd, &relro_fd) < 0) {
      Panic("Could not receive %s relro descriptor from parent", this->name);
    }

    printf("Child received %s relro fd %d\n", this->name, relro_fd);

    int ret = TEMP_FAILURE_RETRY(::read(fd, &this->info, sizeof(this->info)));
    if (ret != static_cast<int>(sizeof(this->info))) {
      Panic("Could not receive %s relro information from parent", this->name);
    }

    this->info.relro_fd = relro_fd;
    printf("Child received %s relro load=%p start=%p size=%p\n",
           this->name,
           (void*)this->info.load_address,
           (void*)this->info.relro_start,
           (void*)this->info.relro_size);
  }

  void UseSharedRelro(crazy_context_t* context) {
    if (!crazy_library_use_relro_sharing(this->library,
                                         this->info.relro_start,
                                         this->info.relro_size,
                                         this->info.relro_fd,
                                         context)) {
      Panic("Could not use %s shared RELRO: %s\n",
            this->name,
            crazy_context_get_error(context));
    }
  }
};

int main() {

  crazy_context_t* context = crazy_context_create();

  Library foo;
  Library bar;

  crazy_context_add_search_path_for_address(context, (void*)&main);

  // Load libfoo_with_relro.so
  crazy_context_set_load_address(context, 0x20000000);
  foo.Init("libfoo_with_relro.so", context);

  crazy_context_set_load_address(context, 0x20800000);
  bar.Init("libbar_with_relro.so", context);

  printf("Libraries loaded\n");

  int pipes[2];
  if (socketpair(AF_UNIX, SOCK_STREAM, 0, pipes) < 0)
    Panic("Could not create socket pair: %s", strerror(errno));

  pid_t child = fork();
  if (child < 0)
    Panic("Could not fork test program!");

  if (child == 0) {
    // In the child.
    printf("Child waiting for foo relro fd\n");

    foo.ReceiveRelroInfo(pipes[0]);
    foo.UseSharedRelro(context);

    printf("Child waiting for bar relro fd\n");
    bar.ReceiveRelroInfo(pipes[0]);
    bar.UseSharedRelro(context);

    printf("RELROs used in child process\n");

    CheckRelroMaps(2);

    FunctionPtr bar_func;
    if (!crazy_library_find_symbol(
             bar.library, "Bar", reinterpret_cast<void**>(&bar_func)))
      Panic("Could not find 'Bar' in library");

    printf("Calling Bar()\n");
    (*bar_func)();

    printf("Bar() called, exiting\n");

    exit(0);

  } else {
    // In the parent.

    printf("Parent enabling foo RELRO sharing\n");

    foo.EnableSharedRelro(context);
    foo.SendRelroInfo(pipes[1]);

    printf("Parent enabling bar RELRO sharing\n");

    bar.EnableSharedRelro(context);
    bar.SendRelroInfo(pipes[1]);

    printf("RELROs enabled and sent to child\n");

    CheckRelroMaps(2);

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

  printf("Closing libraries\n");
  bar.Close();
  foo.Close();

  crazy_context_destroy(context);
  return 0;
}
