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

#include <memory>
#include <unistd.h>
#include "Abcc_host.h"
using namespace abcc;

static int parseArguments(char **argv,
                           std::string &abi, std::string &ndk_dir, std::string &sysroot,
                           std::string &input, std::string &output, std::string &platform, bool &savetemps) {
  unsigned idx = 1;
  savetemps = false;
  while (argv[idx] != 0) {
    std::string arg = argv[idx++];
    if (arg.find("--abi=") != std::string::npos) {
      abi = arg.substr(arg.rfind('=')+1);
      continue;
    }
    if (arg.find("--ndk-dir=") != std::string::npos) {
      ndk_dir = arg.substr(arg.rfind('=')+1);
      continue;
    }
    if (arg.find("--sysroot=") != std::string::npos) {
      sysroot = arg.substr(arg.rfind('=')+1);
      continue;
    }
    if (arg.find("--platform=") != std::string::npos) {
      platform = arg.substr(arg.rfind('=')+1);
      continue;
    }
    if (arg.find("--file") != std::string::npos) {
      input = argv[idx++];
      output = argv[idx++];
      continue;
    }
    if (arg == "--verbose" || arg == "-v") {
      kVerbose = true;
      continue;
    }

    if (arg == "-save-temps") {
      savetemps = true;
      continue;
    }

    LOGE("Unsupport argument: %s", arg.c_str());
    return -1;
  }
  return 0;
}

int main(int argc, char **argv) {
  char buf[256] = {'\0'};
  std::string this_bin = argv[0];
  std::string cwd;
  if (getcwd(buf, 256))
    cwd = buf;
  else {
    LOGE("getcwd failed");
    return -1;
  }
  std::string abi;
  std::string ndk_dir;
  std::string sysroot;
  std::string input;
  std::string output;
  std::string platform;
  std::string working_dir;
  std::string toolchain_bin;
  bool savetemps;

  if (parseArguments(argv,
                     abi, ndk_dir, sysroot, input, output, platform, savetemps) != 0)
    return -1;

  // Make sure they are absolute path
  if (this_bin[0] != '/')
    this_bin = cwd + "/" + this_bin;
  if (input[0] != '/')
    input = cwd + "/" + input;
  if (output[0] != '/')
    output = cwd + "/" + output;

  toolchain_bin = this_bin.substr(0, this_bin.rfind('/'));
  working_dir = input.substr(0, input.rfind('/'));
  if (platform.empty()) // Folow ndk-bc2native.py usage
    platform = "android-9";

  if (abi.empty() || input.empty() || output.empty() || toolchain_bin.empty() || platform.empty()) {
    LOGE("Argument out-of-expect.");
    return -1;
  }

  if (sysroot.empty() && ndk_dir.empty()) {
    LOGE("Must be either standalone or ndk mode");
    return -1;
  }

  if (sysroot.empty()) {
    // NDK mode
    sysroot = ndk_dir + "/platforms/" + platform + "/arch-" + TargetAbi(abi).getArch();
  }


  std::auto_ptr<BitcodeCompiler> compiler;
  if (ndk_dir.empty())
    compiler.reset(new HostBitcodeCompiler(abi, sysroot, toolchain_bin,
                                           input, output,
                                           working_dir, platform, savetemps));
  else
    compiler.reset(new HostBitcodeCompiler(abi, sysroot, ndk_dir, toolchain_bin,
                                           input, output,
                                           working_dir, platform, savetemps));

  compiler->prepare();
  if (compiler->returnCode() != RET_OK) {
    LOGE("prepare failed");
    return -1;
  }

  compiler->cleanupPre();
  if (compiler->returnCode() != RET_OK) {
    LOGE("cleanupPre failed");
    return -1;
  }

  compiler->execute();
  compiler->cleanupPost();

  if (compiler->returnCode() != RET_OK) {
    LOGE("execution failed");
    return -1;
  }

  return 0;
}
