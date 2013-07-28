// Copyright (c) 2013 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include <dlfcn.h>
#include <stdio.h>

extern "C" bool Zoo() {
  printf("%s: Entering\n", __FUNCTION__);
  void* bar_lib = dlopen("libbar.so", RTLD_NOW);
  if (!bar_lib) {
    fprintf(stderr, "Could not libbar.so: %s\n", dlerror());
    return false;
  }
  printf("%s: Opened libbar.so @%p\n", __FUNCTION__, bar_lib);

  void (*bar_func)();

  bar_func = reinterpret_cast<void (*)()>(dlsym(bar_lib, "Bar"));
  if (!bar_func) {
    fprintf(stderr, "Could not find 'Bar' symbol in libbar.so\n");
    return false;
  }
  printf("%s: Found 'Bar' symbol at @%p\n", __FUNCTION__, bar_func);

  // Call it.
  printf("%s: Calling Bar()\n", __FUNCTION__);
  (*bar_func)();

  printf("%s: Closing libbar.so\n", __FUNCTION__);
  dlclose(bar_lib);

  printf("%s: Exiting\n", __FUNCTION__);
  return true;
}
