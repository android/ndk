/*
 * Copyright 2012, The Android Open Source Project
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

#include <jni.h>
#include <android/log.h>
#include <dirent.h>
#include <fcntl.h>
#include <unistd.h>
#include <list>
#include <map>
#include <string>
#include <utility>
#include <vector>
#include <fstream>
#include <pthread.h>

#define LOG_TAG "AbccNative"
#define LOGE(format, ...)  do {\
  __android_log_print(ANDROID_LOG_ERROR, LOG_TAG, format, ##__VA_ARGS__);\
  } while(0)
#define LOGI(format, ...)  do {\
  __android_log_print(ANDROID_LOG_INFO, LOG_TAG, format, ##__VA_ARGS__);\
  } while(0)


enum TargetAbi {
  abi_armeabi = 0,
  abi_armeabi_v7a,
  abi_x86,
  abi_mips,
};

#if defined(__arm__) && defined(__ARM_ARCH_7A__)
TargetAbi abi = abi_armeabi_v7a;
#elif defined(__arm__)
TargetAbi abi = abi_armeabi;
#elif defined(__i686__)
TargetAbi abi = abi_x86;
#elif defined(__mips__)
TargetAbi abi = abi_mips;
#else
# error "Unsupport target"
#endif

enum TargetFieldID {
  target_arch = 0,
  target_triple = 1,
  target_emulation = 2,
  target_extra_cflags = 3,
  target_extra_ldflags = 4,
};

const char* target_data[4][5] = {
  {"arm", "armv5te-linux-androideabi", "armelf_linux_eabi", "", ""},
  {"arm", "thumbv7-linux-androideabi", "armelf_linux_eabi", "", ""},
  {"x86", "i686-linux-android", "elf_i386", "-disable-fp-elim -mattr=-ssse3,-sse41,-sse42,-sse4a,-popcnt", ""},
  {"mips", "mipsel-linux-android", "elf32ltsmip", "", ""},
};

enum ExecutableFieldID {
  exec_translate = 0,
  exec_llc = 1,
  exec_ld = 2,
};

const unsigned NUM_THREADS = 1;
const char *lib_dir;
const char *sysroot;

// Use std::list to easily keep validity while erase
typedef std::vector<std::string> LDFlagsTy;
typedef std::list<std::string> LibsDependedTy;

struct BitcodeInfoTy {
  BitcodeInfoTy() {}
  BitcodeInfoTy(const std::string &bitcode)
    : mBitcode(bitcode), mShared(false) {}

  std::string mBitcode;
  std::string mSOName;
  LDFlagsTy mLDFlags;
  LibsDependedTy mLDLibs; // Will be removed again and again until empty while removeExternalLDLibs
  std::string mLDLibsStr; // Immutable once read in
  bool mShared;
};

typedef std::map<std::string/*soname*/, BitcodeInfoTy> SONameMapTy;

void swapEndian(unsigned char *buffer, size_t n) {
  for (size_t i = 0; i < n/2; ++i) {
    char tmp = buffer[i];
    buffer[i] = buffer[n-i-1];
    buffer[n-i-1] =tmp;
  }
}

int transferBytesToNum(const unsigned char *buffer, size_t n) {
  int ret = 0;
  for (size_t i = 0; i < n; ++i) {
    ret = ret * 0x100 + buffer[i];
  }
  return ret;
}

/*
  The bitcode wrapper definition:

  struct AndroidBitcodeWrapper {
    uint32_t Magic;
    uint32_t Version;
    uint32_t BitcodeOffset;
    uint32_t BitcodeSize;
    uint32_t HeaderVersion;
    uint32_t TargetAPI;
    uint32_t PNaClVersion;
    uint16_t CompilerVersionTag;
    uint16_t CompilerVersionLen;
    uint32_t CompilerVersion;
    uint16_t OptimizationLevelTag;
    uint16_t OptimizationLevelLen;
    uint32_t OptimizationLevel;
  };
*/
#ifdef READ_BITCODE_INFO_FILE
int readInfoFile(BitcodeInfoTy &info) {
  if (info.mBitcode.empty()) {
    LOGE("Must set bitcode name before readInfoFile.");
    return 1;
  }
  info.mSOName = info.mBitcode.substr(info.mBitcode.rfind("/") + 1);
  info.mLDFlags.clear();
  info.mLDLibs.clear();
  info.mLDLibsStr.clear();

  std::string filename(info.mBitcode + ".info");
  std::fstream fin(filename.c_str());
  std::string str;
  while (fin >> str) {
    if (str.find("--sysroot") != std::string::npos) {
      continue; // Ignore on-host sysroot
    }
    if (str == "-o") {  // Ignore on-host output path
      fin >> str;
      continue;
    }
    if (str == "-soname") {  // Ignore on-host output path
      fin >> str;
      info.mSOName = str;
      continue;
    }
    if (str.substr(0, 2) == "-l") {
      info.mLDLibs.push_back(str.substr(2));
      info.mLDLibsStr += std::string(" ") + str;
      continue;
    }
    if (str == "-shared") {
      info.mShared = true;
      continue;
    }
    if (str.size() > 2 &&
        str.substr(str.size()-2) == ".a") {
      str = str.substr(0, str.size()-2);
      str = str.substr(str.rfind("/") + 4);
      str = std::string("-l") + str;
    }
    if (str.size() > 3 &&
        str.substr(str.size()-3) == ".so") {
      str = str.substr(0, str.size()-3);
      str = str.substr(str.rfind("/") + 4);
      str = std::string("-l") + str;
    }

    info.mLDFlags.push_back(str);
  }
  return 0;
}
#else // READ_BITCODE_INFO_FILE
int readWrapper(BitcodeInfoTy &info) {
  if (info.mBitcode.empty()) {
    LOGE("Must set bitcode name before readWrapper.");
    return 1;
  }
  info.mSOName = info.mBitcode.substr(info.mBitcode.rfind("/") + 1);
  info.mLDFlags.clear();
  info.mLDLibs.clear();
  info.mLDLibsStr.clear();

  const char *bc = info.mBitcode.c_str();
  LOGI("Read wrapper for bitcode: %s", bc);
  int fd = open(bc, O_RDONLY);
  unsigned char buffer[5] = {'\0', '\0', '\0', '\0', '\0'};
  read(fd, buffer, 4);  // magic
  swapEndian(buffer, 4);
  int magic = transferBytesToNum(buffer, 4);
  if (magic != 0x0b17c0de) {
    LOGE("Wrong bitcode wrapper");
    close(fd);
    return 1;
  }
  read(fd, buffer, 4);  // version
  read(fd, buffer, 4);  // offset
  swapEndian(buffer, 4);
  int offset = transferBytesToNum(buffer, 4);
  lseek(fd, 4*7, SEEK_SET);
  offset -= 4*7;  // skip
  while (offset > 0) {
    read(fd, buffer, 4);
    swapEndian(buffer, 4);
    uint16_t length = transferBytesToNum(buffer, 2);
    uint16_t tag = transferBytesToNum(buffer+2, 2);
    LOGI("length: %d", length);
    LOGI("tag: %d", tag);
    length = (length + 3) & ~3;

    unsigned char *large_buffer = (unsigned char*) malloc(length+1);
    large_buffer[length] = '\0';
    int n = read(fd, large_buffer, length);
    if (n != length) {
      LOGE("Read bitcode data error");
      close(fd);
      return 1;
    }
    if (tag == 0x5001) {
      swapEndian(large_buffer, length);
      int type = transferBytesToNum(large_buffer, length);
      if (type == /*shared=*/1) {
        info.mShared = true;
      }
    } else if (tag == 0x5002) {
      info.mSOName = reinterpret_cast<char*>(large_buffer);
    } else if (tag == 0x5003) {
      const char *lib = reinterpret_cast<char*>(large_buffer);
      info.mLDLibs.push_back(lib);
      info.mLDLibsStr += std::string(" -l") + lib;
    } else if (tag == 0x5004) {
      swapEndian(large_buffer, length);
      int flag = transferBytesToNum(large_buffer, length);
      if (flag & (1 << 0)) {
        info.mLDFlags.push_back("-z combreloc");
      }
      if (flag & (1 << 1)) {
        info.mLDFlags.push_back("--no-undefined");
      }
      if (flag & (1 << 2)) {
        info.mLDFlags.push_back("-z execstack");
      } else {
        info.mLDFlags.push_back("-z noexecstack");
      }
      if (flag & (1 << 3)) {
        info.mLDFlags.push_back("-z initfirst");
      }
      if (flag & (1 << 4)) {
        info.mLDFlags.push_back("-z interpose");
      }
      if (flag & (1 << 5)) {
        info.mLDFlags.push_back("-z loadfltr");
      }
      if (flag & (1 << 6)) {
        info.mLDFlags.push_back("-z muldefs");
      }
      if (flag & (1 << 7)) {
        info.mLDFlags.push_back("-z nocopyreloc");
      }
      if (flag & (1 << 8)) {
        info.mLDFlags.push_back("-z nodefaultlib");
      }
      if (flag & (1 << 9)) {
        info.mLDFlags.push_back("-z nodelete");
      }
      if (flag & (1 << 10)) {
        info.mLDFlags.push_back("-z nodlopen");
      }
      if (flag & (1 << 11)) {
        info.mLDFlags.push_back("-z nodump");
      }
      if (flag & (1 << 12)) {
        info.mLDFlags.push_back("-z relro");
      }
      if (flag & (1 << 13)) {
        info.mLDFlags.push_back("-z lazy");
      } else {
        info.mLDFlags.push_back("-z now");
      }
      if (flag & (1 << 14)) {
        info.mLDFlags.push_back("-z origin");
      }
    } else {
      // Other field, just omit
    }
    offset -= (length + 4);
    free(large_buffer);
  }
  close(fd);
  return 0;
}
#endif // READ_BITCODE_INFO_FILE

int handleTask(const std::string &cmd, std::string &err_msg) {
  int ret = system(cmd.c_str());
  if (ret != 0) {
    LOGE("Failed command: %s", cmd.c_str());
    err_msg += std::string("Failed command: ") + cmd + std::string("\n");
  }
  return ret;
}

int handleTask(const std::string &cmd) {
  std::string err_msg;
  return handleTask(cmd, err_msg);
}

int getFilesFromDir(const char *dir, std::vector<std::string> &files) {
  files.clear();
  DIR *dp = opendir(dir);
  if (!dp)  return 1;

  struct dirent *entry = 0;
  while ((entry = readdir(dp)) != 0) {
    std::string filename(entry->d_name);
    if (filename == "." || filename == "..")
      continue;

    std::string full_path = std::string(dir) + "/" + filename;
    int fd = open(full_path.c_str(), O_RDONLY);
    if (fd < 0) {
      LOGE("Error opening file: %s", full_path.c_str());
      continue;
    }
    unsigned char buffer[4];
    read(fd, buffer, 4);  // magic
    close(fd);
    swapEndian(buffer, 4);
    int magic = transferBytesToNum(buffer, 4);
    if (magic != 0x0b17c0de) {
      LOGI("Found file %s magic: %x, but we need: %x (Ignored)", full_path.c_str(), magic, 0x0b17c0de);
      continue;
    }

    std::string stem = full_path.substr(0, full_path.rfind("."));
    std::string lib = stem;
    int lib_fd = open(lib.c_str(), O_RDONLY);
    if (lib_fd >= 0) {
      read(lib_fd, buffer, 4);  // magic
      close(lib_fd);
      swapEndian(buffer, 4);
      int magic = transferBytesToNum(buffer, 4);
      if (magic == 0x7f454c46) {
        LOGI("Found file %s magic: %x, No compile for %s", lib.c_str(), magic, full_path.c_str());
        continue;
      }
    }

    lib = stem + ".so";
    lib_fd = open(lib.c_str(), O_RDONLY);
    if (lib_fd >= 0) {
      read(lib_fd, buffer, 4);  // magic
      close(lib_fd);
      swapEndian(buffer, 4);
      int magic = transferBytesToNum(buffer, 4);
      if (magic == 0x7f454c46) {
        LOGI("Found file %s magic: %x, No compile for %s", lib.c_str(), magic, full_path.c_str());
        continue;
      }
    }

    LOGI("Push back a bitcode: %s", full_path.c_str());
    files.push_back(full_path);
  }
  closedir(dp);
  return 0;
}

void removeExternalLDLibs(SONameMapTy &soname_map) {
  for (SONameMapTy::iterator i = soname_map.begin(), e = soname_map.end();
       i != e; ++i) {
    BitcodeInfoTy &info = i->second;
    for (LibsDependedTy::iterator i_libs = info.mLDLibs.begin(),
         e_libs = info.mLDLibs.end(); i_libs != e_libs; ) {
      LibsDependedTy::iterator cur_it = i_libs;
      ++i_libs;  // In case we remove this -l field
      std::string full_soname = std::string("lib") + *cur_it + ".so";
      if (soname_map.find(full_soname) == soname_map.end()) {
        LOGI("Drop -l%s from %s for linking order.", (*cur_it).c_str(), info.mBitcode.c_str());
        info.mLDLibs.erase(cur_it);
      }
    }
  }
}

void* translateBitcode(void *par) {
  std::string bc = (char*)par;
  std::string stem = bc.substr(0, bc.rfind("."));
  std::string target_bc = stem + "-target.bc";

  /*
   * le32-none-ndk-translate
   */
  std::string cmd;
  cmd = std::string("LD_LIBRARY_PATH=") + sysroot + "/usr/lib";
  cmd += std::string(" ") + sysroot + "/usr/bin/le32-none-ndk-translate";
  cmd += std::string(" -arch=") + target_data[abi][target_arch];
  cmd += std::string(" -o ") + target_bc;
  cmd += std::string(" ") + bc;

  handleTask(cmd);
#ifdef NDEBUG
  handleTask(std::string("rm -f ") + bc);
#endif
  return 0;
}

void* compileBitcode(void *par) {
  std::string bc = (char*)par;
  std::string stem = bc.substr(0, bc.rfind("."));
  std::string target_bc = stem + "-target.bc";
  std::string obj = stem + ".o";

  /*
   * llc
   */
  std::string cmd;
  cmd = std::string("LD_LIBRARY_PATH=") + sysroot + "/usr/lib";
  cmd += std::string(" ") + sysroot + "/usr/bin/llc";
  cmd += std::string(" -mtriple=") + target_data[abi][target_triple];
#if defined(__arm__)
  cmd += std::string(" -arm-enable-ehabi -arm-enable-ehabi-descriptors");
#endif
  cmd += " -filetype=obj -relocation-model=pic -code-model=small";
  cmd += " -use-init-array -mc-relax-all -float-abi=soft -O2";
  cmd += std::string(" ") + target_data[abi][target_extra_cflags];
  cmd += std::string(" -o ") + obj;
  cmd += std::string(" ") + target_bc;

  std::string err_msg;
  int ret = handleTask(cmd, err_msg);
  if (ret != 0) {
    LOGE("Cannot compile bitcode (%s): %s", bc.c_str(), cmd.c_str());
    std::fstream fout;
    std::string file = std::string(lib_dir) + "/compile_error";
    fout.open(file.c_str(), std::fstream::out | std::fstream::app);
    fout << err_msg;
    fout.close();
  }
#ifdef NDEBUG
  handleTask(std::string("rm -f ") + target_bc);
#endif
  return 0;
}

int linkBitcode(const BitcodeInfoTy &info) {
  std::string stem(info.mBitcode);
  stem = stem.substr(0, stem.rfind("."));
  std::string obj = stem + ".o";
  std::string lib = stem;
  if (info.mShared) {
    lib += ".so";
  }

  /*
   * ld.mcld
   */
  handleTask(std::string("rm -f ") + lib);
  std::string cmd;
  cmd = std::string("LD_LIBRARY_PATH=") + sysroot + "/usr/lib";
  cmd += std::string(" ") + sysroot + "/usr/bin/ld.mcld";
  cmd += std::string(" --sysroot=") + sysroot;
  if (info.mShared) {
    cmd += " -shared";
  }
  cmd += " -eh-frame-hdr -dynamic-linker /system/bin/linker";
  cmd += std::string(" -X -Bsymbolic -m ") + target_data[abi][target_emulation];
  cmd += std::string(" ") + target_data[abi][target_extra_ldflags];
  if (info.mShared) {
    cmd += std::string(" ") + sysroot + "/usr/lib/crtbegin_so.o";
  } else {
    cmd += std::string(" ") + sysroot + "/usr/lib/crtbegin_dynamic.o";
  }
  if (info.mSOName != "") {
    cmd += std::string(" -soname ") + info.mSOName;
  }
  cmd += std::string(" ") + obj;
  cmd += std::string(" @") + sysroot + "/usr/lib/libportable.wrap " + sysroot + "/usr/lib/libportable.a";
  cmd += std::string(" -L") + lib_dir;
  cmd += std::string(" -L") + sysroot + "/usr/lib";
  cmd += std::string(" -L/system/lib");
  for (LDFlagsTy::const_iterator i = info.mLDFlags.begin(), e = info.mLDFlags.end();
       i != e; ++i) {
    cmd += std::string(" ") + *i;
  }
  // TODO: If this module needs libfoo.so, but since libfoo.so is not a real bitcode under
  //       our trick, it won't be included at module.info. How did we workaround this?
  cmd += info.mLDLibsStr;
  // Replace libgcc by libcompiler-rt + libgabi++
  cmd += std::string(" ") + sysroot + "/usr/lib/libcompiler_rt_static.a";
  cmd += std::string(" ") + sysroot + "/usr/lib/libgabi++_shared.so";
  if (info.mShared) {
    cmd += std::string(" ") + sysroot + "/usr/lib/crtend_so.o";
  } else {
    cmd += std::string(" ") + sysroot + "/usr/lib/crtend_android.o";
  }
  cmd += std::string(" -o ") + lib;

  std::string err_msg;
  int ret = handleTask(cmd, err_msg);
  if (ret != 0) {
    LOGE("Cannot link bitcode (%s): %s", info.mBitcode.c_str(), cmd.c_str());
    std::fstream fout;
    std::string file = std::string(lib_dir) + "/compile_error";
    fout.open(file.c_str(), std::fstream::out | std::fstream::app);
    fout << err_msg;
    fout.close();
    return ret;
  }
  handleTask(std::string("chmod 0755 ") + lib);
#ifdef NDEBUG
  handleTask(std::string("rm -f ") + obj);
#endif
  return 0;
}

int genLibs(const char *lib_dir, const char *sysroot) {
  jint ret = 0;

  std::vector<std::string/*bitcode*/> files;
  ret = getFilesFromDir(lib_dir, files);
  if (ret != 0 || files.empty()) {
    LOGE("Cannot get files from directory: %s", lib_dir);
    return ret;
  }

  // Read bitcode wrapper information
  std::vector<BitcodeInfoTy> bc_infos;
  for (std::vector<std::string>::const_iterator i = files.begin(),
       e = files.end(); i != e; ++i) {
    const std::string &bitcode = *i;
    BitcodeInfoTy bc_info(bitcode);
#ifdef READ_BITCODE_INFO_FILE
    ret = readInfoFile(bc_info);
#else
    ret = readWrapper(bc_info);
#endif
    if (ret != 0) {
      LOGE("Cannot read wrapper for bitcodes.");
      return ret;
    }
    bc_infos.push_back(bc_info);
  }

  // Create mapping from soname to info
  SONameMapTy soname_map;
  for (std::vector<BitcodeInfoTy>::const_iterator i = bc_infos.begin(),
       e = bc_infos.end(); i != e; ++i) {
    const BitcodeInfoTy &info = *i;
    LOGI("Link relationship: %s -> %s (BitcodeInfo)",
         info.mSOName.c_str(), info.mBitcode.c_str());
    soname_map[info.mSOName] = info;
  }

  // Run translate and compile in parallel
  pthread_t tid[NUM_THREADS];

  // Translate
  for (std::vector<BitcodeInfoTy>::const_iterator i = bc_infos.begin(),
       e = bc_infos.end(); i < e; i += NUM_THREADS) {
    unsigned thread_i;
    for (thread_i = 0; thread_i < NUM_THREADS; ++thread_i) {
      if (i + thread_i == e) {
        break;
      }
      const BitcodeInfoTy &info = *(i + thread_i);
      pthread_create(&tid[thread_i], 0, translateBitcode, (void*)info.mBitcode.c_str());
    }

    for (unsigned count = 0; count < thread_i; ++count) {
      pthread_join(tid[count], 0);
    }
  }

  // Compile
  for (std::vector<BitcodeInfoTy>::const_iterator i = bc_infos.begin(),
       e = bc_infos.end(); i < e; i += NUM_THREADS) {
    unsigned thread_i;
    for (thread_i = 0; thread_i < NUM_THREADS; ++thread_i) {
      if (i + thread_i == e) {
        break;
      }
      const BitcodeInfoTy &info = *(i + thread_i);
      pthread_create(&tid[thread_i], 0, compileBitcode, (void*)info.mBitcode.c_str());
    }

    for (unsigned count = 0; count < thread_i; ++count) {
      pthread_join(tid[count], 0);
    }
  }

  // Only linking needs sequential
  // Remove external ldlibs for topological traverse (linking order)
  removeExternalLDLibs(soname_map);

  while (!soname_map.empty()) {
    SONameMapTy::iterator i = soname_map.begin(), e = soname_map.end();
    for (; i != e; ++i) {
      BitcodeInfoTy &info = i->second;

      if (info.mLDLibs.empty()) {
        // No internal dependency for this bitcode
        LOGI("Process bitcode: %s -> %s", info.mBitcode.c_str(), info.mSOName.c_str());
        ret = linkBitcode(info);
        if (ret != 0) {
          LOGE("Process bitcode failed on %s!", info.mBitcode.c_str());
        }

        handleTask(std::string("rm -f ") + info.mBitcode + ".info");
        soname_map.erase(i);
        removeExternalLDLibs(soname_map);
        break;
      }
    }

    if (i == e) {
      LOGE("Failed to compute linking order: Internal cyclic dependency!");
      return 1;
    }
  }

  return ret; //unused
}

#ifdef __cplusplus
extern "C" {
#endif

JNIEXPORT jint JNICALL
Java_compiler_abcc_AbccService_genLibs(JNIEnv *env, jobject thiz,
                                       jstring j_lib_dir, jstring j_sysroot) {
  lib_dir = env->GetStringUTFChars(j_lib_dir, 0);
  LOGI("Working directory: %s", lib_dir);

  sysroot = env->GetStringUTFChars(j_sysroot, 0);
  LOGI("Sysroot: %s", sysroot);

  handleTask(std::string("rm -f ") + lib_dir + "/compile_error");
  genLibs(lib_dir, sysroot);
  handleTask(std::string("cd ") + lib_dir + " && ln -s " + sysroot + "/usr/lib/libgabi++_shared.so libgabi++_shared.so");
  if (handleTask(std::string("ls ") + lib_dir + "/compile_error") != 0) {
    handleTask(std::string("echo 0 > ") + lib_dir + "/compile_result");
    handleTask(std::string("chmod 0644 ") + lib_dir + "/compile_result");
    handleTask(std::string("rm -f ") + lib_dir + "/compile_result");
    return 0;
  } else {
    handleTask(std::string("echo 56 > ") + lib_dir + "/compile_result");
    handleTask(std::string("chmod 0644 ") + lib_dir + "/compile_result");
    handleTask(std::string("chmod 0644 ") + lib_dir + "/compile_error");
    handleTask(std::string("rm -f ") + lib_dir + "/compile_result");
    return 56;
  }
}

#ifdef __cplusplus
} // extern "C"
#endif
