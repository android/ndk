// Copyright (C) 2012 The Android Open Source Project
// All rights reserved.
//
// Redistribution and use in source and binary forms, with or without
// modification, are permitted provided that the following conditions
// are met:
// 1. Redistributions of source code must retain the above copyright
//    notice, this list of conditions and the following disclaimer.
// 2. Redistributions in binary form must reproduce the above copyright
//    notice, this list of conditions and the following disclaimer in the
//    documentation and/or other materials provided with the distribution.
// 3. Neither the name of the project nor the names of its contributors
//    may be used to endorse or promote products derived from this software
//    without specific prior written permission.
//
// THIS SOFTWARE IS PROVIDED BY THE PROJECT AND CONTRIBUTORS ``AS IS'' AND
// ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
// IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
// ARE DISCLAIMED.  IN NO EVENT SHALL THE PROJECT OR CONTRIBUTORS BE LIABLE
// FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL
// DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS
// OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION)
// HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT
// LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY
// OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF
// SUCH DAMAGE.
//===----------------------------------------------------------------------===//
//
//                     The LLVM Compiler Infrastructure
//
// This file is dual licensed under the MIT and the University of Illinois Open
// Source Licenses. See LICENSE.TXT for details.
//
//
//  This file implements the "Exception Handling APIs"
//  http://www.codesourcery.com/public/cxx-abi/abi-eh.html
//  http://www.intel.com/design/itanium/downloads/245358.htm
//
//===----------------------------------------------------------------------===//

#include <cstdlib>
#include "dwarf_helper.h"

namespace __cxxabiv1 {

  uintptr_t readULEB128(const uint8_t** data) {
    uintptr_t result = 0;
    uintptr_t shift = 0;
    unsigned char byte;
    const uint8_t *p = *data;
    do {
      byte = *p++;
      result |= static_cast<uintptr_t>(byte & 0x7F) << shift;
      shift += 7;
    } while (byte & 0x80);
    *data = p;
    return result;
  }

  intptr_t readSLEB128(const uint8_t** data) {
    uintptr_t result = 0;
    uintptr_t shift = 0;
    unsigned char byte;
    const uint8_t *p = *data;
    do {
      byte = *p++;
      result |= static_cast<uintptr_t>(byte & 0x7F) << shift;
      shift += 7;
    } while (byte & 0x80);
    *data = p;
    if ((byte & 0x40) && (shift < (sizeof(result) << 3))) {
      result |= static_cast<uintptr_t>(~0) << shift;
    }
    return static_cast<intptr_t>(result);
  }

  uintptr_t readEncodedPointer(const uint8_t** data,
                               uint8_t encoding) {
    uintptr_t result = 0;
    if (encoding == DW_EH_PE_omit) {
      return result;
    }
    const uint8_t* p = *data;

    switch (encoding & 0x0F) {
    default:
      abort();
      break;
    case DW_EH_PE_absptr:
      result = *((uintptr_t*)p);
      p += sizeof(uintptr_t);
      break;
    case DW_EH_PE_uleb128:
      result = readULEB128(&p);
      break;
    case DW_EH_PE_sleb128:
      result = static_cast<uintptr_t>(readSLEB128(&p));
      break;
    case DW_EH_PE_udata2:
      result = *((uint16_t*)p);
      p += sizeof(uint16_t);
      break;
    case DW_EH_PE_udata4:
      result = *((uint32_t*)p);
      p += sizeof(uint32_t);
      break;
    case DW_EH_PE_udata8:
      result = static_cast<uintptr_t>(*((uint64_t*)p));
      p += sizeof(uint64_t);
      break;
    case DW_EH_PE_sdata2:
      result = static_cast<uintptr_t>(*((int16_t*)p));
      p += sizeof(int16_t);
      break;
    case DW_EH_PE_sdata4:
      result = static_cast<uintptr_t>(*((int32_t*)p));
      p += sizeof(int32_t);
      break;
    case DW_EH_PE_sdata8:
      result = static_cast<uintptr_t>(*((int64_t*)p));
      p += sizeof(int64_t);
      break;
    }

    switch (encoding & 0x70) {
    default:
      abort();
      break;
    case DW_EH_PE_absptr:
      break;
    case DW_EH_PE_pcrel:
      if (result) {
        result += (uintptr_t)(*data);
      }
      break;
    }

    // finally, apply indirection
    if (result && (encoding & DW_EH_PE_indirect)) {
      result = *(reinterpret_cast<uintptr_t*>(result));
    }
    *data = p;
    return result;
  }

} // namespace __cxxabiv1
