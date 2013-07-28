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

    CheckMaps(2);

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

    CheckMaps(2);

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
