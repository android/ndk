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

#ifndef ABCC_ABCC_HOST_H
#define ABCC_ABCC_HOST_H

#include <cstdio>
#include "Abcc.h"

#define LOG(format, ...) do { \
  fprintf(stderr, format, ##__VA_ARGS__); \
  fprintf(stderr, "\n"); \
} while(0)

extern bool kVerbose;
#define LOGE  LOG
#define LOGV(format, ...) do {\
  if (kVerbose) LOG(format, ##__VA_ARGS__); \
} while(0)

namespace abcc {

class HostBitcodeCompiler : public BitcodeCompiler {
  std::string mIn;
  std::string mOut;
  std::string mNDKDir;  // empty string if standalone mode
  std::string mPlatform;
  std::string mToolchainBinPath;  // Used at ndk mode to prevent tedious path issue
  std::map<std::string, std::string> mRuntimePath; // mappings of library name and full path

public:
  HostBitcodeCompiler(const std::string &abi, const std::string &sysroot, const std::string &toolchain_bin,
                      const std::string &input, const std::string &output, const std::string &working_dir,
                      const std::string &platform, const bool savetemps, bool bit32 = true);
  HostBitcodeCompiler(const std::string &abi, const std::string &sysroot, const std::string &ndk_dir, const std::string &toolchain_bin,
                      const std::string &input, const std::string &output, const std::string &working_dir,
                      const std::string &platform, const bool savetemps, bool bit32 = true);

public:
  virtual int parseLDFlags(BitcodeInfo &info, const std::string &str);

private:
  virtual void getBitcodeFiles();
  virtual void prepareToolchain();
  virtual void copyRuntime(const BitcodeInfo &info);
  virtual void removeIntermediateFile(const std::string &path);
  void initRuntimePath();
  const std::string getRuntimePath(const std::string &libname);

private:
  const std::string getToolchainBinPath() const;
  const std::string getCompilerRTPath() const;
  const std::string getGAbixxPath() const;
  const std::string getLibPortablePath() const;
  const std::string getGCCUnwindPath() const;
};

} // namespace abcc

#endif
