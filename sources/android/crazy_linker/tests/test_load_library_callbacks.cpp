// Copyright (c) 2014 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

// A crazy linker test to test callbacks for delayed execution.

#include <stdio.h>
#include <crazy_linker.h>

#include "test_util.h"

typedef void (*FunctionPtr)();

namespace {

bool PostCallback(crazy_callback_t* callback, void* poster_opaque) {
  printf("Post callback, poster_opaque %p, handler %p, opaque %p\n",
         poster_opaque,
         callback->handler,
         callback->opaque);

  // Copy callback to the address given in poster_opaque.
  crazy_callback_t* request =
      reinterpret_cast<crazy_callback_t*>(poster_opaque);
  *request = *callback;

  return true;
}

void CheckAndRunCallback(crazy_callback_t* callback) {
  printf("Run callback, handler %p, opaque %p\n",
         callback->handler,
         callback->opaque);

  if (!callback->handler) {
    Panic("Post for delayed execution not invoked\n");
  }

  // Run the callback, then clear it for re-use.
  crazy_callback_run(callback);
  memset(callback, 0, sizeof(*callback));
}

}  // namespace

int main() {
  crazy_context_t* context = crazy_context_create();
  crazy_library_t* library;

  // DEBUG
  crazy_context_set_load_address(context, 0x20000000);

  // Create a new callback, initialized to NULLs.
  crazy_callback_t callback = {NULL, NULL};

  // Set a callback poster that copies its callback to &callback.
  crazy_context_set_callback_poster(context, &PostCallback, &callback);

  crazy_callback_poster_t poster;
  void* poster_opaque;

  // Check that the API returns the values we set.
  crazy_context_get_callback_poster(context, &poster, &poster_opaque);
  if (poster != &PostCallback || poster_opaque != &callback) {
    Panic("Get callback poster error\n");
  }

  // Load libfoo.so.
  if (!crazy_library_open(&library, "libfoo.so", context)) {
    Panic("Could not open library: %s\n", crazy_context_get_error(context));
  }
  CheckAndRunCallback(&callback);

  // Find the "Foo" symbol.
  FunctionPtr foo_func;
  if (!crazy_library_find_symbol(
           library, "Foo", reinterpret_cast<void**>(&foo_func))) {
    Panic("Could not find 'Foo' in libfoo.so\n");
  }

  // Call it.
  (*foo_func)();

  // Close the library.
  crazy_library_close_with_context(library, context);
  CheckAndRunCallback(&callback);

  crazy_context_destroy(context);

  return 0;
}
