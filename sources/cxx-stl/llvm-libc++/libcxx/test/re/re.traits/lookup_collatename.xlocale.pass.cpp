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
//   lookup_collatename(ForwardIterator first, ForwardIterator last) const;

#include <regex>
#include <iterator>
#include <cassert>
#include "test_iterators.h"

template <class char_type>
void
test(const char_type* A, const std::basic_string<char_type>& expected)
{
    std::regex_traits<char_type> t;
    typedef forward_iterator<const char_type*> F;
    assert(t.lookup_collatename(F(A), F(A + t.length(A))) == expected);
}

int main()
{
#if !defined(__ANDROID__)
    std::locale::global(std::locale("cs_CZ.ISO8859-2"));
    test("ch", std::string("ch"));

    std::locale::global(std::locale("cs_CZ.ISO8859-2"));
    test(L"ch", std::wstring(L"ch"));
#endif
}
