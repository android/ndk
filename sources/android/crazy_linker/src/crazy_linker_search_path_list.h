// Copyright (c) 2013 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef CRAZY_LINKER_SEARCH_PATH_LIST_H
#define CRAZY_LINKER_SEARCH_PATH_LIST_H

#include <string.h>

#include "crazy_linker_util.h"  // for String

namespace crazy {

// A simple class to model a list of search paths, and perform
// file system probing with it.
class SearchPathList {
 public:
  SearchPathList() : list_(), env_list_(), full_path_() {}

  // Reset the list, i.e. make it empty.
  void Reset();

  // Reset the list from an environment variable value.
  void ResetFromEnv(const char* var_name);

  // Add one or more paths to the list.
  // |path_list| contains a list of paths separated by columns.
  // |path_list_end| points after the list's last character.
  void AddPaths(const char* path_list, const char* path_list_end);

  // Convenience function that takes a 0-terminated string.
  void AddPaths(const char* path_list) {
    AddPaths(path_list, path_list + ::strlen(path_list));
  }

  // Try to find a file named |file_name| by probing the file system
  // with every item in the list as a suffix. On success, returns the
  // full path string, or NULL on failure.
  const char* FindFile(const char* file_name);

 private:
  String list_;
  String env_list_;
  String full_path_;
};

}  // namespace crazy

#endif  // CRAZY_LINKER_SEARCH_PATH_LIST_H