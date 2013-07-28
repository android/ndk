// Copyright (c) 2013 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

// A crazy linker test to:
// - Load a library (libfoo.so) with the linker.
// - Find the address of the "Foo" function in it.
// - Call the function.
// - Close the library.

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

typedef void (*FunctionPtr)();

static void Panic(const char* fmt, ...) {
  va_list args;
  fprintf(stderr, "PANIC: ");
  va_start(args, fmt);
  vfprintf(stderr, fmt, args);
  va_end(args);
  exit(1);
}

static pthread_mutex_t s_lock;
static pthread_cond_t s_cond;

static void CheckMaps(int expected_count) {
  printf("Checking for %d RELROs in /proc/self/maps\n", expected_count);

  FILE* file = fopen("/proc/self/maps", "rb");
  if (!file)
    Panic("Could not open /proc/self/maps (pid %d): %s\n",
          getpid(),
          strerror(errno));

  char line[512];
  int count_relros = 0;
  printf("proc/%d/maps:\n", getpid());
  while (fgets(line, sizeof line, file)) {
    if (strstr(line, "with_relro")) {
      printf("%s", line);
      if (strstr(line, "/dev/ashmem/RELRO:"))
        count_relros++;
    }
  }
  fclose(file);

  if (count_relros != expected_count)
    Panic(
        "Invalid shared RELRO sections in /proc/self/maps: %d"
        " (expected %d)\n",
        count_relros,
        expected_count);

  printf("RELRO count check ok!\n");
}

// Send a file descriptor |fd| through |socket|.
// Return 0 on success, -1/errno on failure.
static int SendFd(int socket, int fd) {
  struct iovec iov;

  char buffer[1];
  buffer[0] = 0;

  iov.iov_base = buffer;
  iov.iov_len = 1;

  struct msghdr msg;
  struct cmsghdr* cmsg;
  char cms[CMSG_SPACE(sizeof(int))];

  ::memset(&msg, 0, sizeof(msg));
  msg.msg_iov = &iov;
  msg.msg_iovlen = 1;
  msg.msg_control = reinterpret_cast<caddr_t>(cms);
  msg.msg_controllen = CMSG_LEN(sizeof(int));

  cmsg = CMSG_FIRSTHDR(&msg);
  cmsg->cmsg_len = CMSG_LEN(sizeof(int));
  cmsg->cmsg_level = SOL_SOCKET;
  cmsg->cmsg_type = SCM_RIGHTS;
  ::memcpy(CMSG_DATA(cmsg), &fd, sizeof(int));

  int ret = sendmsg(socket, &msg, 0);
  if (ret < 0)
    return -1;

  if (ret != iov.iov_len) {
    errno = EIO;
    return -1;
  }

  return 0;
}

static int ReceiveFd(int socket, int* fd) {
  char buffer[1];
  struct iovec iov;

  iov.iov_base = buffer;
  iov.iov_len = 1;

  struct msghdr msg;
  struct cmsghdr* cmsg;
  char cms[CMSG_SPACE(sizeof(int))];

  ::memset(&msg, 0, sizeof msg);
  msg.msg_name = 0;
  msg.msg_namelen = 0;
  msg.msg_iov = &iov;
  msg.msg_iovlen = 1;

  msg.msg_control = reinterpret_cast<caddr_t>(cms);
  msg.msg_controllen = sizeof(cms);

  int ret = recvmsg(socket, &msg, 0);
  if (ret < 0)
    return -1;
  if (ret == 0) {
    errno = EIO;
    return -1;
  }

  cmsg = CMSG_FIRSTHDR(&msg);
  ::memcpy(fd, CMSG_DATA(cmsg), sizeof(int));
  return 0;
}

int main() {

  // Initialize process-shared mutex.
  {
    pthread_mutexattr_t attr;
    pthread_mutexattr_init(&attr);
    pthread_mutexattr_setpshared(&attr, PTHREAD_PROCESS_SHARED);
    pthread_mutex_init(&s_lock, &attr);
    pthread_mutexattr_destroy(&attr);
  }
  // Initialize process-shared condition variable.
  {
    pthread_condattr_t attr;
    pthread_condattr_init(&attr);
    pthread_condattr_setpshared(&attr, PTHREAD_PROCESS_SHARED);
    pthread_cond_init(&s_cond, &attr);
    pthread_condattr_destroy(&attr);
  }

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
      pthread_cond_signal(&s_cond);
      Panic("Could not use RELRO sharing: %s",
            crazy_context_get_error(context));
    }

    printf("RELRO used in child process\n");

    CheckMaps(1);

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

    CheckMaps(1);

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
