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
                                         const std::string &platform)
  : BitcodeCompiler(abi, sysroot, working_dir), mIn(input), mOut(output),
    mNDKDir(""), mPlatform(platform) {
}

HostBitcodeCompiler::HostBitcodeCompiler(const std::string &abi, const std::string &sysroot, const std::string &ndk_dir, const std::string &toolchain_bin,
                                         const std::string &input, const std::string &output, const std::string &working_dir,
                                         const std::string &platform)
  : BitcodeCompiler(abi, sysroot, working_dir), mIn(input), mOut(output),
    mNDKDir(ndk_dir), mPlatform(platform), mToolchainBinPath(toolchain_bin) {
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
      info.mLDLibs.push_back(str.substr(2));
      info.mLDLibsStr += " " + str;
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

  cmd = " @" + getLibPortablePath() + "/libportable.wrap " + getLibPortablePath() + "/libportable.a";
  cmd += " " + getCompilerRTPath() + "/libcompiler_rt_static.a";
  cmd += " " + getGAbixxPath() + "/libgabi++_shared.so";
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
