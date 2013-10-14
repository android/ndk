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

#include <sstream>
#include "Abcc_host.h"
using namespace abcc;

bool kVerbose = false;

HostBitcodeCompiler::HostBitcodeCompiler(const std::string &abi, const std::string &sysroot,
                                         const std::string &input, const std::string &output, const std::string &working_dir,
                                         const std::string &platform, const bool savetemps)
  : BitcodeCompiler(abi, sysroot, working_dir, savetemps), mIn(input), mOut(output),
    mNDKDir(""), mPlatform(platform) {
  initRuntimePath();
}

HostBitcodeCompiler::HostBitcodeCompiler(const std::string &abi, const std::string &sysroot, const std::string &ndk_dir, const std::string &toolchain_bin,
                                         const std::string &input, const std::string &output, const std::string &working_dir,
                                         const std::string &platform, const bool savetemps)
  : BitcodeCompiler(abi, sysroot, working_dir, savetemps), mIn(input), mOut(output),
    mNDKDir(ndk_dir), mPlatform(platform), mToolchainBinPath(toolchain_bin) {
  initRuntimePath();
}

void HostBitcodeCompiler::initRuntimePath() {
  mRuntimePath.clear();

  mRuntimePath.insert(std::make_pair("gabi++_static", getGAbixxPath() + "/libgabi++_static.a"));
  mRuntimePath.insert(std::make_pair("gabi++_shared", getGAbixxPath() + "/libgabi++_shared.so"));
  mRuntimePath.insert(std::make_pair("compiler_rt_static", getCompilerRTPath() + "/libcompiler_rt_static.a"));
  mRuntimePath.insert(std::make_pair("portable", getLibPortablePath() + "/libportable.a"));
  mRuntimePath.insert(std::make_pair("portable.wrap", getLibPortablePath() + "/libportable.wrap"));
  mRuntimePath.insert(std::make_pair("gccunwind", getGCCUnwindPath() + "/libgccunwind.a"));
}

const std::string HostBitcodeCompiler::getRuntimePath(const std::string &libname) {
  if (mRuntimePath.count(libname)) {
    return mRuntimePath.find(libname)->second;
  }
  return "";
}

int HostBitcodeCompiler::parseLDFlags(BitcodeInfo &info, const std::string &orig_str) {
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

    if (str.size() > 2 &&
        str.substr(str.size() - 2) == ".a") {
      info.mLDLibs.push_back(str);
      info.mLDLibsStr += " " + str;
      continue;
    }
    if (str.size() > 3 &&
        str.substr(str.size() - 3) == ".so") {
      info.mLDLibs.push_back(str);
      info.mLDLibsStr += " " + str;
      continue;
    }

    // Parse -lxxx
    if (str.size() > 2 &&
        str.substr(0, 2) == "-l") {
      std::string runtime_path = getRuntimePath(str.substr(2));
      if (!runtime_path.empty()) {
        info.mLDLibsStr += " " + runtime_path;
      }
      else {
        info.mLDLibs.push_back(str.substr(2));
        info.mLDLibsStr += " " + str;
      }
      continue;
    }

    // Some other flags, like --no-undefined, -z now, -z noexecstack, ...
    info.mLDFlags += str + " ";
  } // while
  return 0;
}

void HostBitcodeCompiler::getBitcodeFiles() {
  BitcodeInfo bcinfo(mIn);
  if (bcinfo.readWrapper(*this) != 0) {
    LOGE("Cannot read wrapper for bitcode %s", mIn.c_str());
    mRet = RET_FAIL_PREPARE_BITCODE;
    return;
  }

  bcinfo.mOutPath = mOut;
  mBitcodeFiles.push_back(bcinfo);
}

void HostBitcodeCompiler::prepareToolchain() {
  // le32-none-ndk-translate
  std::string cmd = getToolchainBinPath() + "/le32-none-ndk-translate";
  mExecutableToolsPath[(unsigned)CMD_TRANSLATE] = cmd;

  // llc
  cmd = getToolchainBinPath() + "/llc";
  mExecutableToolsPath[(unsigned)CMD_COMPILE] = cmd;

  // ld.mcld
  cmd = getToolchainBinPath() + "/ld.mcld";
  cmd += " -L" + mSysroot + "/usr/lib";
  mExecutableToolsPath[(unsigned)CMD_LINK] = cmd;

  cmd = " @" + getRuntimePath("portable.wrap") + " " + getRuntimePath("portable");
  cmd += " " + getRuntimePath("compiler_rt_static");
  cmd += " " + getRuntimePath("gccunwind");
  cmd += " -ldl";
  mExecutableToolsPath[(unsigned)CMD_LINK_RUNTIME] = cmd;
}

void HostBitcodeCompiler::copyRuntime(const BitcodeInfo &info) {
}

void HostBitcodeCompiler::removeIntermediateFile(const std::string &path) {
  if (!kVerbose)
    runCmd(std::string("rm -f ") + path);
}

const std::string HostBitcodeCompiler::getToolchainBinPath() const {
  if (!mNDKDir.empty())
    return mToolchainBinPath;
  else
    return mSysroot + "/usr/bin";
}

const std::string HostBitcodeCompiler::getCompilerRTPath() const {
  if (!mNDKDir.empty())
    return std::string(mNDKDir) + "/sources/android/compiler-rt/libs/" + (const char*)mAbi;
  else
    return mSysroot + "/usr/lib";
}

const std::string HostBitcodeCompiler::getGAbixxPath() const {
  if (!mNDKDir.empty())
    return std::string(mNDKDir) + "/sources/cxx-stl/gabi++/libs/" + (const char*)mAbi;
  else
    return mSysroot + "/usr/lib";
}

const std::string HostBitcodeCompiler::getLibPortablePath() const {
  if (!mNDKDir.empty())
    return std::string(mNDKDir) + "/sources/android/libportable/libs/" + (const char*)mAbi;
  else
    return mSysroot + "/usr/lib";
}

const std::string HostBitcodeCompiler::getGCCUnwindPath() const {
  if (!mNDKDir.empty())
    return std::string(mNDKDir) + "/sources/android/gccunwind/libs/" + (const char*)mAbi;
  else
    return mSysroot + "/usr/lib";
}
