/*
 * Copyright (C) 2016 The Android Open Source Project
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *      http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */

#include <shaderc/shaderc.hpp>
#include <iostream>

int main() {
  shaderc::Compiler compiler;
  if (!compiler.IsValid()) return -1;

  static const char* source_text =
      "#version 310 es\n"
      "layout(location = 0) in highp vec4 vtxColor;\n"
      "layout(location = 0) out highp vec4 outColor;\n"
      "void main() {\n"
      "   outColor = vtxColor;\n"
      "}";

  shaderc::SpvCompilationResult result =
      compiler.CompileGlslToSpv(source_text, strlen(source_text),
                       shaderc_glsl_fragment_shader, "input.glsl");

  if (0u != result.GetNumErrors()) {
    std::cerr << result.GetErrorMessage() << std::endl;
    return -1;
  }
  if (0u != result.GetNumWarnings()) {
    std::cerr << result.GetErrorMessage() << std::endl;
    return -1;
  }
  if (shaderc_compilation_status_success != result.GetCompilationStatus()) {
    std::cerr << "Bad compilation result" << std::endl;
    return -1;
  }
  if (result.cbegin() == result.cend()) {
    std::cerr << "No data returned" << std::endl;
    return -1;
  }
  std::cout << "Success" << std::endl;
  return 0;
}

