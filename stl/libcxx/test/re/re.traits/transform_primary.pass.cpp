// -*- C++ -*-
//===----------------------------------------------------------------------===//
//
//                     The LLVM Compiler Infrastructure
//
// This file is dual licensed under the MIT and the University of Illinois Open
// Source Licenses. See LICENSE.TXT for details.
//
//===----------------------------------------------------------------------===//

// <regex>

// template <class charT> struct regex_traits;

// template <class ForwardIterator>
//   string_type
//   transform_primary(ForwardIterator first, ForwardIterator last) const;

#include <regex>
#include <cassert>
#include "test_iterators.h"

int main()
{
    {
        std::regex_traits<char> t;
        const char A[] = "A";
        const char Aacute[] = "\xC1";
        typedef forward_iterator<const char*> F;
        assert(t.transform_primary(F(A), F(A+1)) !=
               t.transform_primary(F(Aacute), F(Aacute+1)));
/* Disable locale specific tests on Android because Android's NDK does not
 * support locales other than "C" and "POSIX".
 *
 * https://code.google.com/p/android/issues/detail?id=57313
 */
#if !defined(__ANDROID__)
        t.imbue(std::locale("cs_CZ.ISO8859-2"));
        assert(t.transform_primary(F(A), F(A+1)) ==
               t.transform_primary(F(Aacute), F(Aacute+1)));
#endif
    }
    {
        std::regex_traits<wchar_t> t;
        const wchar_t A[] = L"A";
        const wchar_t Aacute[] = L"\xC1";
        typedef forward_iterator<const wchar_t*> F;
        assert(t.transform_primary(F(A), F(A+1)) !=
               t.transform_primary(F(Aacute), F(Aacute+1)));
/* Disable locale specific tests on Android because Android's NDK does not
 * support locales other than "C" and "POSIX".
 *
 * https://code.google.com/p/android/issues/detail?id=57313
 */
#if !defined(__ANDROID__)
        t.imbue(std::locale("cs_CZ.ISO8859-2"));
        assert(t.transform_primary(F(A), F(A+1)) ==
               t.transform_primary(F(Aacute), F(Aacute+1)));
#endif
    }
}
