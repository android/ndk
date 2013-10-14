/*
 * Copyright 2013, The Android Open Source Project
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *     http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */

#ifndef ABCC_ABCC_DEVICE_H
#define ABCC_ABCC_DEVICE_H

#include <android/log.h>
#if VERBOSE
#include <sys/time.h>
#endif
#include "Abcc.h"

#define LOG_TAG "AbccNative"
#define LOGE(format, ...)  do {\
  __android_log_print(ANDROID_LOG_ERROR, LOG_TAG, format, ##__VA_ARGS__);\
  } while(0)

#if VERBOSE
#define LOGV(format, ...)  do {\
  __android_log_print(ANDROID_LOG_VERBOSE, LOG_TAG, format, ##__VA_ARGS__);\
  } while(0)
#else
#define LOGV(format, ...)
#endif  // VERBOSE

namespace abcc {

#if VERBOSE
class Timer {
private:
  struct timeval _t0;
public:
  Timer() {};
  void start() { gettimeofday(&_t0, NULL); }
  long long stop() {
  struct timeval t;
    gettimeofday(&t, NULL);
    long long l0 = _t0.tv_sec * 1000000LL + _t0.tv_usec;
    long long l = t.tv_sec * 1000000LL + t.tv_usec;
    return l - l0;
  }
};

extern long long llc_usec;

#endif // VERBOSE


class DeviceBitcodeCompiler : public BitcodeCompiler {
public:
  DeviceBitcodeCompiler(const std::string &working_dir, const std::string &sysroot);
  virtual void cleanupPost();

public:
  virtual int parseLDFlags(BitcodeInfo &info, const std::string &str);

private:
  virtual void getBitcodeFiles();
  virtual void prepareToolchain();
  virtual void copyRuntime(const BitcodeInfo &info);
  virtual void removeIntermediateFile(const std::string &path);
};

} // namespace abcc

#endif
