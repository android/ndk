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

#ifndef ABCC_ABCC_H
#define ABCC_ABCC_H

#include <list>
#include <map>
#include <string>
#include <vector>

namespace abcc {

enum ReturnCode {
  RET_OK,
  RET_FAIL_RUN_CMD,
  RET_FAIL_UNSUPPORT_ABI,
  RET_FAIL_PREPARE_BITCODE,
  RET_FAIL_PREPARE_TOOL,
  RET_FAIL_CLEANUP,
  RET_FAIL_TRANSLATE,
  RET_FAIL_COMPILE,
  RET_FAIL_LINK
};

enum Command {
  CMD_TRANSLATE = 0,
  CMD_COMPILE,
  CMD_LINK,
  CMD_LINK_RUNTIME
};

class TargetAbi {
public:
  enum Abi {
    ARMEABI = 0,
    ARMEABI_V7A,
    X86,
    MIPS,
    ARM64_V8A,
    X86_64,
    MIPS64
  };

private:
  Abi mAbi;

public:
  TargetAbi(const std::string &abi);
  operator int() const { return (int)mAbi; }
  operator const char*() const {
    if (mAbi == ARMEABI)  return "armeabi";
    if (mAbi == ARMEABI_V7A)  return "armeabi-v7a";
    if (mAbi == X86)  return "x86";
    if (mAbi == MIPS)  return "mips";
    if (mAbi == ARM64_V8A)  return "arm64-v8a";
    if (mAbi == X86_64)  return "x86_64";
    if (mAbi == MIPS64)  return "mips64";
    return 0;
  }
  const char* getArch() const {
    if (mAbi == ARMEABI || mAbi == ARMEABI_V7A)  return "arm";
    if (mAbi == X86)  return "x86";
    if (mAbi == MIPS) return "mips";
    if (mAbi == ARM64_V8A)  return "arm64";
    if (mAbi == X86_64)  return "x86_64";
    if (mAbi == MIPS64)  return "mips64";
    return 0;
  }
};

struct TargetAttributes {
  const char *mArch;
  const char *mTriple;
  const char *mLinkEmulation;
  const char *mBaseCFlags;
  const char *mBaseLDFlags;
};

const TargetAttributes kGlobalTargetAttrs[] = {
  {"arm", "armv5te-linux-androideabi", "armelf_linux_eabi", "-arm-enable-ehabi -arm-enable-ehabi-descriptors -float-abi=soft", "-dynamic-linker /system/bin/linker"},
#ifdef FORCE_ARM
  {"arm", "armv7-linux-androideabi", "armelf_linux_eabi", "-arm-enable-ehabi -arm-enable-ehabi-descriptors -float-abi=soft", "-dynamic-linker /system/bin/linker"},
#else
  {"arm", "thumbv7-linux-androideabi", "armelf_linux_eabi", "-arm-enable-ehabi -arm-enable-ehabi-descriptors -float-abi=soft", "-dynamic-linker /system/bin/linker"},
#endif
  {"x86", "i686-linux-android", "elf_i386", "-disable-fp-elim -force-align-stack -mattr=-ssse3,-sse41,-sse42,-sse4a,-popcnt -x86-force-gv-stack-cookie", "-dynamic-linker /system/bin/linker"},
  {"mips", "mipsel-linux-android", "elf32ltsmip", "-float-abi=hard", "-dynamic-linker /system/bin/linker"},
  {"arm64", "aarch64-linux-android", "aarch64linux", "", "-dynamic-linker /system/bin/linker64"},
  {"x86_64", "x86_64-linux-android", "elf_x86_64", "-disable-fp-elim -force-align-stack -mattr=+sse3 -x86-force-gv-stack-cookie", "-dynamic-linker /system/bin/linker64"},
  {"mips64", "mips64el-linux-android", "elf64ltsmip", "", "-dynamic-linker /system/bin/linker64"},
};

// Used when computing mutual dependency
class BitcodeCompiler;
class BitcodeInfo;
typedef std::map<std::string/*soname*/, BitcodeInfo> SONameMap;

class BitcodeInfo {
public:
  BitcodeInfo() {}  // Only for stl use
  BitcodeInfo(const std::string &bc);
  int readWrapper(BitcodeCompiler &);
  static void dropExternalLDLibs(SONameMap &map);

  bool mShared;
  int mOptimizationLevel;
  std::string mBCPath;
  std::string mTargetBCPath;
  std::string mObjPath;
  std::string mOutPath;
  std::string mSOName;
  std::string mLDFlags; // --no-undefined, ...
  std::string mLDLocalLibsStr;  // i.e.: ./obj/local/.../libxxx.a
  std::list<std::string> mLDLibs;  // -lxxx, will be removed one-by-one until empty
  std::string mLDLibsStr; // Immutable once read in

public:
  static int transferBytesToNumLe(const unsigned char *buffer, size_t n);
};


class BitcodeCompiler {
protected:
  TargetAbi mAbi;
  std::string mSysroot;
  std::string mWorkingDir;

  // Target-independent, but global
  std::string mGlobalCFlags;
  std::string mGlobalLDFlags;
  std::string mGlobalLDLibs;

  ReturnCode mRet;
  std::vector<BitcodeInfo> mBitcodeFiles;
  SONameMap mSonameMap;
  std::string mExecutableToolsPath[(unsigned)CMD_LINK_RUNTIME+1];

public:
  BitcodeCompiler(const std::string &abi, const std::string &sysroot, const std::string &working_dir, const bool savetemps);
  virtual ~BitcodeCompiler();
  const ReturnCode returnCode() const { return mRet; }
  virtual void cleanupPre() {}
  virtual void cleanupPost() {}
  void prepare() {
    prepareBitcodes();
    if (returnCode() != RET_OK)
      return;

    prepareToolchain();
    if (returnCode() != RET_OK)
      return;
  }
  void execute() {
    translate();
    if (returnCode() != RET_OK)
      return;

    compile();
    if (returnCode() != RET_OK)
      return;

    link();
    if (returnCode() != RET_OK)
      return;
  }

private:
  bool mSaveTemps;

  void prepareBitcodes();
  void createSONameMapping();
  virtual void getBitcodeFiles() = 0;
  virtual void prepareToolchain() = 0;
  virtual void copyRuntime(const BitcodeInfo &info) = 0;
  virtual void removeIntermediateFile(const std::string &path) = 0;
  void translate();
  void compile();
  void link();

public:
  virtual int parseLDFlags(BitcodeInfo &info, const std::string &str) = 0;

protected:
  void runCmd(std::string cmd, bool dump = false);
};

} // namespace abcc

// FIXME: We use LOGV, LOGE in Abcc.cpp, how to prevent this anti dependency?
#if ON_DEVICE
#include "Abcc_device.h"
#else
#include "Abcc_host.h"
#endif

#endif
