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

#include <dirent.h>
#include <fcntl.h>
#include <unistd.h>
#include <sstream>
#include <sys/stat.h>
#if ENABLE_PARALLEL_LLVM_CG
#include <cpu-features.h>
#endif
#include "Abcc_device.h"
using namespace abcc;

#if defined(__arm__) && defined(__ARM_ARCH_7A__)
# define CURRENT_ABI  "armeabi-v7a"
#elif defined(__arm__)
# define CURRENT_ABI  "armeabi"
#elif defined(__i386__)
# define CURRENT_ABI  "x86"
#elif defined(__mips__)
# define CURRENT_ABI  "mips"
#elif defined(__aarch64__)
# define CURRENT_ABI "arm64-v8a"
#elif defined(__x86_64__)
# define CURRENT_ABI "x86_64"
#elif defined(__mips64)
# define CURRENT_ABI "mips64"
#else
# error "Unsupport target abi"
#endif


DeviceBitcodeCompiler::DeviceBitcodeCompiler(const std::string &working_dir, const std::string &sysroot)
  : BitcodeCompiler(CURRENT_ABI, sysroot, working_dir, false/*!savetemps*/) {}

void DeviceBitcodeCompiler::cleanupPost() {
  ReturnCode ret = mRet;
  std::string cmd("echo ");
  if (ret == RET_OK)
    cmd += "0";
  else
    cmd += "255";
  cmd += " > " + mWorkingDir + "/compile_result";
  runCmd(cmd);

  if (ret == RET_OK) {
    cmd = "rm -f " + mWorkingDir + "/compile_log";
    runCmd(cmd);
  }
  cmd = "chmod 0755 " + mWorkingDir + "/*";
  runCmd(cmd);
  mRet = ret; // Restore execution return code
}

int DeviceBitcodeCompiler::parseLDFlags(BitcodeInfo &info, const std::string &orig_str) {
  std::stringstream ss(orig_str);
  std::string str;
  while (ss >> str) {
    if (str.find("--sysroot") != std::string::npos) {
      continue;
    }
    if (str == "-o") {
      ss >> str;
      continue;
    }
    if (str == "-soname") {
      ss >> str;
      info.mSOName = str;
      continue;
    }
    if (str == "-shared") {
      info.mShared = true;
      info.mOutPath += ".so";
      continue;
    }
    if (str == "-static") {
      info.mStatic = true;
    }

    // Parse -lxxx
    if (str.size() > 2 &&
        str.substr(0, 2) == "-l") {
      info.mLDLibs.push_back(str.substr(2));
      info.mLDLibsStr += " " + str;
      continue;
    }

    // Some other flags, like --no-undefined, -z now, -z noexecstack, ...
    info.mLDFlags += str + " ";
  } // while
  return 0;
}

void DeviceBitcodeCompiler::getBitcodeFiles() {
  std::vector<std::string> bc_files,lib_files;
  DIR *dp = opendir(mWorkingDir.c_str());
  if (!dp) {
    LOGE("Error opening working dir: %s", mWorkingDir.c_str());
    mRet = RET_FAIL_PREPARE_BITCODE;
    return;
  }

  struct dirent *entry = 0;
  while ((entry = readdir(dp)) != 0) {
    std::string filename(entry->d_name);
    std::string full_path = mWorkingDir + "/" + filename;
    if (filename == "." || filename == "..")
      continue;

    std::string libpath = mSysroot + "/usr/lib/" + filename.substr(0, filename.rfind('.')) + ".so";
    struct stat buf;
    if (stat(libpath.c_str(), &buf) == 0) {
      // This file has the same name in our runtime library pool.  Add to deletion list.
      lib_files.push_back(full_path);
      continue;
    }

    int fd = open(full_path.c_str(), O_RDONLY);
    if (fd < 0) {
      LOGV("Error opening file: %s (Ignored)", full_path.c_str());
      continue;
    }

    unsigned char buffer[4];
    read(fd, buffer, 4);
    close(fd);
    int magic = BitcodeInfo::transferBytesToNumLe(buffer, 4);

    if (magic != 0x0b17c0de) {
      LOGV("Found file %s magic: %x, but we need a wrapped bitcode.", full_path.c_str(), magic);
      continue;
    }

    LOGV("Push_back a bitcode: %s", full_path.c_str());
    bc_files.push_back(full_path);
  } // while
  closedir(dp);

  if (bc_files.empty()) {
    LOGV("No bitcodes needs to compile");
    return;
  }

  if (returnCode() != RET_OK) {
    LOGV("Cannot get bitcode files from directory: %s", mWorkingDir.c_str());
    return;
  }

  // Reomve lib file
  for (std::vector<std::string>::const_iterator i = lib_files.begin(),
       e = lib_files.end(); i != e; ++i) {
      std::string cmd = "rm -f " + *i;
      runCmd(cmd.c_str());
  }

  for (std::vector<std::string>::const_iterator i = bc_files.begin(),
       e = bc_files.end(); i != e; ++i) {
    BitcodeInfo bcinfo(*i);
    if (bcinfo.readWrapper(*this) != 0) {
      LOGE("Cannot read wrapper for bitcode %s", i->c_str());
      mRet = RET_FAIL_PREPARE_BITCODE;
      return;
    }
    mBitcodeFiles.push_back(bcinfo);
  }
}

void DeviceBitcodeCompiler::prepareToolchain() {
  // le32-none-ndk-translate
  std::string cmd = std::string("LD_LIBRARY_PATH=") + mSysroot + "/usr/lib";
  cmd += " " + mSysroot + "/usr/bin/le32-none-ndk-translate";
  mExecutableToolsPath[(unsigned)CMD_TRANSLATE] = cmd;

  // llc
  cmd = std::string("LD_LIBRARY_PATH=") + mSysroot + "/usr/lib";
  cmd += " " + mSysroot + "/usr/bin/llc";
#ifdef ENABLE_PARALLEL_LLVM_CG
  std::ostringstream stream;
  stream << android_getCpuCount();
  cmd += " -thread=" + stream.str();
#endif
  mExecutableToolsPath[(unsigned)CMD_COMPILE] = cmd;

  // ld.mcld
  cmd = std::string("LD_LIBRARY_PATH=") + mSysroot + "/usr/lib";
  cmd += " " + mSysroot + "/usr/bin/ld.mcld";
  cmd += " -L" + mWorkingDir;
  cmd += " -L" + mSysroot + "/usr/lib";

  if (mAbi == TargetAbi::X86_64 || mAbi == TargetAbi::ARM64_V8A || mAbi == TargetAbi::MIPS64)
    cmd += " -L/system/lib64";
  else
    cmd += " -L/system/lib";

  mExecutableToolsPath[(unsigned)CMD_LINK] = cmd;

  cmd = " @" + mSysroot + "/usr/lib/libportable.wrap " + mSysroot + "/usr/lib/libportable.a";
  cmd += " " + mSysroot + "/usr/lib/libcompiler_rt_static.a";
  cmd += " " + mSysroot + "/usr/lib/libgccunwind.a";
  mExecutableToolsPath[(unsigned)CMD_LINK_RUNTIME] = cmd;
}

void DeviceBitcodeCompiler::copyRuntime(const BitcodeInfo &info) {

  std::stringstream ss(info.mLDLibsStr);
  std::string deplib;
  while (ss >> deplib) {
    if (deplib.substr(0, 2) == "-l") {
      std::string libname = "lib" + deplib.substr(2) + ".so";
      std::string libpath = mSysroot + "/usr/lib/" + libname;
      struct stat buf;
      if (stat(libpath.c_str(), &buf) == 0) {
        // Found!
        LOGV("Copy runtime library: %s", libname.c_str());
        runCmd("cp -f " + libpath + " " + mWorkingDir + "/" + libname);
      }
    }
  }
}

void DeviceBitcodeCompiler::removeIntermediateFile(const std::string &path) {
  runCmd(std::string("rm -f ") + path);
}
