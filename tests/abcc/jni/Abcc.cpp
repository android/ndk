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

#include <cstdlib>
#include <fcntl.h>
#include <unistd.h>
#include <stdint.h>
#include "Abcc.h"
using namespace abcc;

TargetAbi::TargetAbi(const std::string &abi) {
  if (abi == "armeabi")
    mAbi = ARMEABI;
  else if (abi == "x86")
    mAbi = X86;
  else if (abi == "mips")
    mAbi = MIPS;
  else
    mAbi = ARMEABI_V7A;  // Default
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

  unsigned char buffer[5] = {'\0', '\0', '\0', '\0', '\0'};
  read(fd, buffer, 4);  // Skip magic number, we have checked
  read(fd, buffer, 4);  // version
  read(fd, buffer, 4);  // offset
  swapEndian(buffer, 4);
  int offset = transferBytesToNum(buffer, 4);
  lseek(fd, 4*7, SEEK_SET);
  offset -= 4*7;  // Useless, skip

  while (offset > 0) {
    read(fd, buffer, 4);
    swapEndian(buffer, 4);
    uint16_t length = transferBytesToNum(buffer, 2);
    uint16_t tag = transferBytesToNum(buffer+2, 2);
    LOGV("length: %d", length);
    LOGV("tag: %d", tag);
    length = (length + 3) & ~3;

    unsigned char *large_buffer = (unsigned char*) malloc(length+1);
    if (large_buffer == 0) {
      LOGE("Cannot create buffer for wrapper field");
      close(fd);
      return -1;
    }
    large_buffer[length] = '\0';
    int n = read(fd, large_buffer, length);
    if (n != length) {
      LOGE("Read wrapper field error");
      close(fd);
      return -1;
    }

    if (tag == 0x5002) {
      const char* ldflags = reinterpret_cast<char*>(large_buffer);
      LOGV("Wrapper field: %s", ldflags);
      if (compiler.parseLDFlags(*this, ldflags) != 0) {
        LOGE("Cannot parse ldflags from wrapper");
        close(fd);
        return -1;
      }
    } else {
      // Some field maybe useful, but we use wrapper to encode command line,
      // this is not necessary for now.
    }
    offset -= (length + 4);
    free(large_buffer);
  } // while
  close(fd);
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

void BitcodeInfo::swapEndian(unsigned char *buffer, size_t n) {
  // We uses le32, so it must be LITTLE ENDIAN
  for (size_t i = 0; i < n/2; ++i) {
    char tmp = buffer[i];
    buffer[i] = buffer[n-i-1];
    buffer[n-i-1] =tmp;
  }
}

int BitcodeInfo::transferBytesToNum(const unsigned char *buffer, size_t n) {
  int ret = 0;
  for (size_t i = 0; i < n; ++i) {
    ret = ret * 0x100 + buffer[i];
  }
  return ret;
}


BitcodeCompiler::BitcodeCompiler(const std::string &abi, const std::string &sysroot, const std::string &working_dir)
  : mAbi(abi), mSysroot(sysroot), mWorkingDir(working_dir), mRet(RET_OK) {
  // CFlags
  mGlobalCFlags = kGlobalTargetAttrs[mAbi].mBaseCFlags;
  mGlobalCFlags += std::string(" -mtriple=") + kGlobalTargetAttrs[mAbi].mTriple;
  mGlobalCFlags += " -filetype=obj -relocation-model=pic -code-model=small";
  mGlobalCFlags += " -use-init-array -mc-relax-all";

#ifdef DEBUG
  mGlobalCFlags += std::string(" ") + "-O0";
#else
  mGlobalCFlags += std::string(" ") + "-O2";
#endif

  if (mAbi == TargetAbi::ARMEABI || mAbi == TargetAbi::ARMEABI_V7A)
    mGlobalCFlags += std::string(" ") + "-arm-enable-ehabi -arm-enable-ehabi-descriptors -float-abi=soft";

  // LDFlags
  mGlobalLDFlags = kGlobalTargetAttrs[mAbi].mBaseLDFlags;
  mGlobalLDFlags += std::string(" -X -m ") + kGlobalTargetAttrs[mAbi].mLinkEmulation;
  mGlobalLDFlags += std::string(" --sysroot=") + mSysroot;
  mGlobalLDFlags += " -eh-frame-hdr -dynamic-linker /system/bin/linker";

  // LDLibs
  mGlobalLDLibs += "";
}

void BitcodeCompiler::translate() {
  for (std::vector<BitcodeInfo>::const_iterator i = mBitcodeFiles.begin(),
       e = mBitcodeFiles.end(); i != e; ++i) {
    const BitcodeInfo &bc = *i;
    std::string cmd = mExecutableToolsPath[(unsigned)CMD_TRANSLATE];
    cmd += std::string(" -arch=") + kGlobalTargetAttrs[mAbi].mArch;
    cmd += " " + bc.mBCPath + " -o " + bc.mTargetBCPath;
    runCmd(cmd);
    if (returnCode() != RET_OK) {
      mRet = RET_FAIL_TRANSLATE;
      return;
    }

    removeIntermediateFile(bc.mBCPath);
  }
}

void BitcodeCompiler::compile() {
  for (std::vector<BitcodeInfo>::const_iterator i = mBitcodeFiles.begin(),
       e = mBitcodeFiles.end(); i != e; ++i) {
    const BitcodeInfo &bc = *i;
    std::string cmd = mExecutableToolsPath[(unsigned)CMD_COMPILE];
    cmd += " " + mGlobalCFlags;
    cmd += " " + bc.mTargetBCPath + " -o " + bc.mObjPath;
    runCmd(cmd);
    if (returnCode() != RET_OK) {
      mRet = RET_FAIL_COMPILE;
      return;
    }

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
        LOGV("Process bitcode: %s -> %s", bc.mBCPath.c_str(), bc.mSOName.c_str());
        std::string cmd = mExecutableToolsPath[(unsigned)CMD_LINK];
        cmd += " " + mGlobalLDFlags;
        cmd += " " + bc.mLDFlags;
        if (bc.mShared) {
          cmd += std::string(" ") + mSysroot + "/usr/lib/crtbegin_so.o";
          cmd += " -shared " + bc.mObjPath + " -o " + bc.mOutPath;
          cmd += " -soname " + bc.mSOName;
        } else {
          cmd += std::string(" ") + mSysroot + "/usr/lib/crtbegin_dynamic.o";
          cmd += " " + bc.mObjPath + " -o " + bc.mOutPath;
        }
        // Add ldlibs
        cmd += " " + mGlobalLDLibs;
        cmd += " " + bc.mLDLibsStr;
        cmd += " " + mExecutableToolsPath[(unsigned)CMD_LINK_RUNTIME];
        if (bc.mShared)
          cmd += std::string(" ") + mSysroot + "/usr/lib/crtend_so.o";
        else
          cmd += std::string(" ") + mSysroot + "/usr/lib/crtend_android.o";
        runCmd(cmd);
        if (returnCode() != RET_OK)
          return;

        cmd = "chmod 0755 " + bc.mOutPath;
        runCmd(cmd);

        copyRuntime(bc);

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
  if (dump) {
    std::string dump_cmd = "echo '" + cmd + "' >> " + mWorkingDir + "/compile_log";
    system(dump_cmd.c_str());
  }
  LOGV("Command: %s", cmd.c_str());
  int ret = system(cmd.c_str());
  if (ret != 0)
    mRet = RET_FAIL_RUN_CMD;
  else
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
