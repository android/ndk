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
#include <jni.h>
#include "Abcc_device.h"
using namespace abcc;

#if VERBOSE
long long abcc::llc_usec;
#endif

#ifdef __cplusplus
extern "C" {
#endif

JNIEXPORT jint JNICALL
Java_compiler_abcc_AbccService_genLibs(JNIEnv *env, jobject thiz,
                                       jstring j_lib_dir, jstring j_sysroot) {
  const char *lib_dir = env->GetStringUTFChars(j_lib_dir, 0);
  LOGV("Working directory: %s", lib_dir);

  const char *sysroot = env->GetStringUTFChars(j_sysroot, 0);
  LOGV("Sysroot: %s", sysroot);


  std::auto_ptr<BitcodeCompiler> compiler(new DeviceBitcodeCompiler(lib_dir, sysroot));
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

#if VERBOSE
  Timer t;
  t.start();
  llc_usec = 0;
#endif

  compiler->execute();

#if VERBOSE
  long long elapsed_msec = (t.stop() + 500) / 1000;
  long long llc_msec = (llc_usec + 500) / 1000;
  LOGV("Elapsed time: %lld.%03ds (llc = %lld.%03ds)", elapsed_msec/1000, (int)elapsed_msec%1000,
           llc_msec/1000, (int)llc_msec%1000);
#endif

  compiler->cleanupPost();

  if (compiler->returnCode() != RET_OK) {
    LOGE("execution failed");
    return -1;
  }

  return 0;
}


#ifdef __cplusplus
} // extern "C"
#endif
