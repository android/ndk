// Copyright (c) 2013 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

// This source file must contain a static destructor to check that
// the crazy linker can resolve weak symbols from the C library,
// like __aeabi_atexit(), which are not normally returned by
// a call to dlsym().

#include <stdlib.h>

extern "C" void __aeabi_atexit(void*);

class A {
 public:
  A() {
    x_ = rand();
    const char* env = getenv("TEST_VAR");
    if (!env || strcmp(env, "INIT"))
      putenv("TEST_VAR=LOAD_ERROR");
    else
      putenv("TEST_VAR=LOADED");
  }

  ~A() {
    const char* env = getenv("TEST_VAR");
    if (!env || strcmp(env, "LOADED"))
      putenv("TEST_VAR=UNLOAD_ERROR");
    else
      putenv("TEST_VAR=UNLOADED");
  }

  int Get() const { return x_; }

 private:
  int x_;
};

A s_a;

extern "C" int Foo() { return s_a.Get(); }
