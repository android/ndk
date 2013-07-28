// Copyright (c) 2013 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "crazy_linker_search_path_list.h"

#include <string.h>

#include "crazy_linker_debug.h"
#include "crazy_linker_system.h"

namespace crazy {

void SearchPathList::Reset() {
  list_.Resize(0);
  env_list_.Resize(0);
  full_path_.Resize(0);
}

void SearchPathList::ResetFromEnv(const char* var_name) {
  Reset();
  const char* env = GetEnv(var_name);
  if (env && *env)
    env_list_ = env;
}

void SearchPathList::AddPaths(const char* list, const char* list_end) {
  // Append a column to the current list, if necessary
  if (list_.size() > 0 && list_[list_.size() - 1] != ':')
    list_ += ':';
  list_.Append(list, list_end - list);
}

const char* SearchPathList::FindFile(const char* file_name) {
  // Sanity checks.
  if (!file_name || !*file_name)
    return NULL;

  LOG("%s: Looking for %s\n", __FUNCTION__, file_name);

  // Build full list by appending the env_list_ after the regular one.
  String full_list = list_;
  if (env_list_.size() > 0) {
    if (full_list.size() > 0 && full_list[full_list.size() - 1] != ':')
      full_list += ':';
    full_list += env_list_;
  }

  // Iterate over all items in the list.
  const char* p = full_list.c_str();
  const char* end = p + full_list.size();

  while (p < end) {
    // compute current list item, and next item start at the same time.
    const char* item = p;
    const char* item_end =
        reinterpret_cast<const char*>(memchr(p, ':', end - p));
    if (item_end)
      p = item_end + 1;
    else {
      item_end = end;
      p = end;
    }

    full_path_.Assign(item, item_end - item);

    // Add trailing directory separator if needed.
    if (full_path_.size() > 0 && full_path_[full_path_.size() - 1] != '/')
      full_path_ += '/';

    full_path_ += file_name;

    if (PathIsFile(full_path_.c_str())) {
      LOG("    FOUND %s\n", full_path_.c_str());
      return full_path_.c_str();
    } else
      LOG("    skip  %s\n", full_path_.c_str());
  }

  return NULL;
}

}  // namespace crazy
