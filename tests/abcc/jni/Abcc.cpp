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

#include <cassert>
#include <cstdlib>
#include <fcntl.h>
#include <fstream>
#include <sstream>
#include <stdint.h>
#include <unistd.h>
#include "Abcc.h"

#if !defined(_WIN32)
#include <sys/mman.h>
#else
#include "mman.h"
#endif

#include <sys/types.h>
#include <sys/stat.h>
using namespace abcc;

TargetAbi::TargetAbi(const std::string &abi) {
  if (abi == "armeabi-v7a" || abi == "armeabi-v7a-hard") //ToDo: support armeabi-v7a-hard
    mAbi = ARMEABI_V7A;
  else if (abi == "armeabi")
    mAbi = ARMEABI;
  else if (abi == "x86")
    mAbi = X86;
  else if (abi == "mips")
    mAbi = MIPS;
  else if (abi == "arm64-v8a")
    mAbi = ARM64_V8A;
  else if (abi == "x86_64")
    mAbi = X86_64;
  else if (abi == "mips64")
    mAbi = MIPS64;
  else {
    assert (false && "Unknown abi for abcc. Check your --abi flag.");
    exit (1);
  }
}


BitcodeInfo::BitcodeInfo(const std::string &bc)
  : mShared(false), mBCPath(bc) {
  std::string stem = mBCPath.substr(0, mBCPath.rfind("."));
  mTargetBCPath = stem + "-target.bc";
  mObjPath = stem + ".o";
  mOutPath = stem;  // If shared, we will add .so after readWrapper
  mSOName = mBCPath.substr(mBCPath.rfind("/") + 1);
}

int BitcodeInfo::readWrapper(BitcodeCompiler &compiler) {
  int fd = open(mBCPath.c_str(), O_RDONLY);

  if (fd < 0) {
    return -1;
  }

  unsigned char *buf, *p;
  struct stat st;
  int bc_offset;

  fstat (fd, &st);
  buf = (unsigned char *) mmap (NULL, st.st_size, PROT_READ, MAP_PRIVATE, fd, 0);
  close (fd);

  bc_offset = transferBytesToNumLe (buf+ 8, 4);
  p = buf + 4 * 7;      // Offset to tag fields.

  while (p < buf + bc_offset) {
    uint16_t tag, length;

    tag = transferBytesToNumLe (p, 2);
    length = transferBytesToNumLe (p + 2, 2);
    p += 4;
    switch (tag) {
      case 0x4002:       // Optimization Level,. e.g., 2 for -O2
        mOptimizationLevel = transferBytesToNumLe (p, 4);
        LOGV("Wrapper field: -O%d", mOptimizationLevel);
        break;

      case 0x5002:       // LDFLAGS string
        LOGV("Wrapper field: %s", p);
        if (compiler.parseLDFlags (*this, reinterpret_cast<const char *>(p)) != 0) {
          LOGE("Cannot parse ldflags from wrapper");
          close(fd);
          return -1;
        }
        break;

      case 0x4001:       // Compiler Version, e.g., 3300 for llvm-3.3.
      case 0x5001:       // Bitcode Type, e.g., rel, shared, or exec.
      default:
        // Some field maybe useful, but we use wrapper to encode command line,
        // this is not necessary for now.
        break;
    }

    p += (length + 3) & ~3;  // Data are always padding to 4-byte boundary.
  }

  munmap (buf, st.st_size);
  return 0;
}

void BitcodeInfo::dropExternalLDLibs(SONameMap &map) {
  for (SONameMap::iterator i = map.begin(), e = map.end(); i != e; ++i) {
    BitcodeInfo &info = i->second;
    for (std::list<std::string>::iterator i_libs = info.mLDLibs.begin(),
         e_libs = info.mLDLibs.end(); i_libs != e_libs; ) {
      std::list<std::string>::iterator cur_libs = i_libs++;
      std::string full_soname = std::string("lib") + *cur_libs + ".so";
      if (map.find(full_soname) == map.end()) {
        LOGV("Drop -l%s from %s for linking order decision", cur_libs->c_str(), info.mSOName.c_str());
        info.mLDLibs.erase(cur_libs);
      }
    }
  }
}

// This function reads N bytes from BUFFER in little endian.
int BitcodeInfo::transferBytesToNumLe(const unsigned char *buffer, size_t n) {
  int ret = 0;
  const unsigned char *p = buffer + n;

  while (--p >= buffer)
    ret = ret * 0x100 + *p;

  return ret;
}

BitcodeCompiler::BitcodeCompiler(const std::string &abi, const std::string &sysroot, const std::string &working_dir, const bool savetemps)
  : mAbi(abi), mSysroot(sysroot), mWorkingDir(working_dir), mRet(RET_OK), mSaveTemps(savetemps) {
  // CFlags
  mGlobalCFlags = kGlobalTargetAttrs[mAbi].mBaseCFlags;
  mGlobalCFlags += std::string(" -mtriple=") + kGlobalTargetAttrs[mAbi].mTriple;
  mGlobalCFlags += " -filetype=obj -mc-relax-all";
  mGlobalCFlags += " -relocation-model=pic -code-model=small -use-init-array";
  mGlobalCFlags += " -ffunction-sections";

  // LDFlags
  mGlobalLDFlags = kGlobalTargetAttrs[mAbi].mBaseLDFlags;
  mGlobalLDFlags += std::string(" -Bsymbolic -X -m ") + kGlobalTargetAttrs[mAbi].mLinkEmulation;
  mGlobalLDFlags += std::string(" --sysroot=") + mSysroot;
  mGlobalLDFlags += " --build-id --eh-frame-hdr";

  // LDLibs
  mGlobalLDLibs = " ";
}

BitcodeCompiler::~BitcodeCompiler() {
}

void BitcodeCompiler::translate() {
  for (std::vector<BitcodeInfo>::const_iterator i = mBitcodeFiles.begin(),
       e = mBitcodeFiles.end(); i != e; ++i) {
    const BitcodeInfo &bc = *i;
    LOGV("Translate bitcode: %s -> %s", bc.mBCPath.c_str(), bc.mTargetBCPath.c_str());
    std::string cmd = mExecutableToolsPath[(unsigned)CMD_TRANSLATE];
    cmd += std::string(" -arch=") + kGlobalTargetAttrs[mAbi].mArch;
    cmd += " " + bc.mBCPath + " -o " + bc.mTargetBCPath;
    runCmd(cmd, /*dump=*/true);
    if (returnCode() != RET_OK) {
      mRet = RET_FAIL_TRANSLATE;
      return;
    }
    if (!mSaveTemps)
      removeIntermediateFile(bc.mBCPath);
  }
}

void BitcodeCompiler::compile() {
  for (std::vector<BitcodeInfo>::const_iterator i = mBitcodeFiles.begin(),
       e = mBitcodeFiles.end(); i != e; ++i) {
    const BitcodeInfo &bc = *i;
    LOGV("Compile bitcode: %s -> %s", bc.mTargetBCPath.c_str(), bc.mObjPath.c_str());
    std::ostringstream os;

    os << mExecutableToolsPath[(unsigned)CMD_COMPILE]
       << " " << mGlobalCFlags
       << " -O" << bc.mOptimizationLevel
       << " " << bc.mTargetBCPath
       << " -o " << bc.mObjPath;

    if (bc.mLDFlags.find("-pie") != std::string::npos)
      os << " -enable-pie";

#if ON_DEVICE && VERBOSE
    Timer t_llc;
    t_llc.start();
#endif
    runCmd(os.str(), /*dump=*/true);
#if ON_DEVICE && VERBOSE
    llc_usec += t_llc.stop();
#endif
    if (returnCode() != RET_OK) {
      mRet = RET_FAIL_COMPILE;
      return;
    }
    if (!mSaveTemps)
      removeIntermediateFile(bc.mTargetBCPath);
  }
}

void BitcodeCompiler::link() {
  BitcodeInfo::dropExternalLDLibs(mSonameMap);

  while (!mSonameMap.empty()) {
    SONameMap::iterator i = mSonameMap.begin(), e = mSonameMap.end();
    for (; i != e; ++i) {
      const BitcodeInfo &bc = i->second;

      if (bc.mLDLibs.empty()) {
        // No internal dependency for this bitcode
        LOGV("Link: %s -> %s", bc.mObjPath.c_str(), bc.mSOName.c_str());
        std::string cmd = mExecutableToolsPath[(unsigned)CMD_LINK];
        std::string libdir = (mAbi == TargetAbi::X86_64) ? "lib64" : "lib";
        cmd += " " + mGlobalLDFlags;
        cmd += " " + bc.mLDFlags;
        if (bc.mShared) {
          cmd += std::string(" ") + mSysroot + "/usr/" + libdir + "/crtbegin_so.o";
          cmd += " -shared " + bc.mObjPath + " -o " + bc.mOutPath;
          cmd += " -soname " + bc.mSOName;
        } else {
          cmd += std::string(" ") + mSysroot + "/usr/" + libdir + "/crtbegin_dynamic.o";
          cmd += " " + bc.mObjPath + " -o " + bc.mOutPath;
        }
        // Add ldlibs
        cmd += " " + bc.mLDLocalLibsStr;
        cmd += " " + mGlobalLDLibs;
        cmd += " " + bc.mLDLibsStr;
        cmd += " " + mExecutableToolsPath[(unsigned)CMD_LINK_RUNTIME];
        if (bc.mShared)
          cmd += std::string(" ") + mSysroot + "/usr/" + libdir + "/crtend_so.o";
        else
          cmd += std::string(" ") + mSysroot + "/usr/" + libdir + "/crtend_android.o";
        runCmd(cmd, /*dump=*/true);
        if (returnCode() != RET_OK)
          return;

        copyRuntime(bc);
        if (!mSaveTemps)
          removeIntermediateFile(bc.mObjPath);

        mSonameMap.erase(i);
        BitcodeInfo::dropExternalLDLibs(mSonameMap);
        break;  // Re-compute
      }
    } // for

    if (i == e) {
      LOGE("Failed to compute linking order: Internal cyclic dependency!");
      mRet = RET_FAIL_LINK;
      return;
    }
  } // while
}

void BitcodeCompiler::runCmd(std::string cmd, bool dump) {
  LOGV("Command: %s", cmd.c_str());
  std::string logfilename = mWorkingDir + "/compile_log";
  if (dump) {
    cmd += " > " + logfilename + " 2>&1";
  }
  int ret = system(cmd.c_str());
  if (ret != 0) {
    mRet = RET_FAIL_RUN_CMD;
    if (dump) {
      std::ifstream ifs(logfilename.c_str());
      std::stringstream sstr;
      sstr << ifs.rdbuf();
      LOGE("Error message: %s", sstr.str().c_str());
      std::fstream fout;
      std::string file = mWorkingDir + "/compile_error";
      fout.open(file.c_str(), std::fstream::out | std::fstream::app);
      fout << "Failed command: " << cmd << "\n";
      fout << "Error message: " << sstr.str() << "\n";
      fout.close();
    }
    return;
  }
  mRet = RET_OK;
}

void BitcodeCompiler::prepareBitcodes() {
  getBitcodeFiles();
  createSONameMapping();
}

void BitcodeCompiler::createSONameMapping() {
  for (std::vector<BitcodeInfo>::const_iterator i = mBitcodeFiles.begin(),
       e = mBitcodeFiles.end(); i != e; ++i) {
    const BitcodeInfo &info = *i;
    LOGV("Map soname %s -> %s", info.mSOName.c_str(), info.mBCPath.c_str());
    mSonameMap[info.mSOName] = info;
  }
}
