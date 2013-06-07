// -*- C++ -*-
//===-------------------- support/win32/locale_win32.cpp ------------------===//
//
//                     The LLVM Compiler Infrastructure
//
// This file is dual licensed under the MIT and the University of Illinois Open
// Source Licenses. See LICENSE.TXT for details.
//
//===----------------------------------------------------------------------===//

#include <ctype.h>

// NOTE: A standalone forward declaration is required to ensure that this
// variable is properly exported with a C name. In other words, this does
// _not_ work:
//
//  extern "C" {
//  const char* const _ctype_android = ...;
//  }
//
extern "C" const char* const _ctype_android;

// This is a local copy of the Bionic _ctype_ array to be used
// by libc++. This is needed because the Bionic version of this
// table misses the _B flag on the TAB (7) character.
static const char ctype_android_tab[256] = {
        _C,     _C,     _C,     _C,     _C,     _C,     _C,     _C|(char)_B,
        _C,     _C|_S,  _C|_S,  _C|_S,  _C|_S,  _C|_S,  _C,     _C,
        _C,     _C,     _C,     _C,     _C,     _C,     _C,     _C,
        _C,     _C,     _C,     _C,     _C,     _C,     _C,     _C,
   _S|(char)_B, _P,     _P,     _P,     _P,     _P,     _P,     _P,
        _P,     _P,     _P,     _P,     _P,     _P,     _P,     _P,
        _N,     _N,     _N,     _N,     _N,     _N,     _N,     _N,
        _N,     _N,     _P,     _P,     _P,     _P,     _P,     _P,
        _P,     _U|_X,  _U|_X,  _U|_X,  _U|_X,  _U|_X,  _U|_X,  _U,
        _U,     _U,     _U,     _U,     _U,     _U,     _U,     _U,
        _U,     _U,     _U,     _U,     _U,     _U,     _U,     _U,
        _U,     _U,     _U,     _P,     _P,     _P,     _P,     _P,
        _P,     _L|_X,  _L|_X,  _L|_X,  _L|_X,  _L|_X,  _L|_X,  _L,
        _L,     _L,     _L,     _L,     _L,     _L,     _L,     _L,
        _L,     _L,     _L,     _L,     _L,     _L,     _L,     _L,
        /* determine printability based on the IS0 8859 8-bit standard */
        _L,     _L,     _L,     _P,     _P,     _P,     _P,     _C,

        _C,     _C,     _C,     _C,     _C,     _C,     _C,     _C, /* 80 */
        _C,     _C,     _C,     _C,     _C,     _C,     _C,     _C, /* 88 */
        _C,     _C,     _C,     _C,     _C,     _C,     _C,     _C, /* 90 */
        _C,     _C,     _C,     _C,     _C,     _C,     _C,     _C, /* 98 */
        _P,     _P,     _P,     _P,     _P,     _P,     _P,     _P, /* A0 */
        _P,     _P,     _P,     _P,     _P,     _P,     _P,     _P, /* A8 */
        _P,     _P,     _P,     _P,     _P,     _P,     _P,     _P, /* B0 */
        _P,     _P,     _P,     _P,     _P,     _P,     _P,     _P, /* B8 */
        _P,     _P,     _P,     _P,     _P,     _P,     _P,     _P, /* C0 */
        _P,     _P,     _P,     _P,     _P,     _P,     _P,     _P, /* C8 */
        _P,     _P,     _P,     _P,     _P,     _P,     _P,     _P, /* D0 */
        _P,     _P,     _P,     _P,     _P,     _P,     _P,     _P, /* D8 */
        _P,     _P,     _P,     _P,     _P,     _P,     _P,     _P, /* E0 */
        _P,     _P,     _P,     _P,     _P,     _P,     _P,     _P, /* E8 */
        _P,     _P,     _P,     _P,     _P,     _P,     _P,     _P, /* F0 */
        _P,     _P,     _P,     _P,     _P,     _P,     _P,     _P  /* F8 */
};

const char* const _ctype_android = ctype_android_tab;
