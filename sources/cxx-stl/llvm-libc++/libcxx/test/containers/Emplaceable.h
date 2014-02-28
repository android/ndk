//===----------------------------------------------------------------------===//
//
//                     The LLVM Compiler Infrastructure
//
// This file is dual licensed under the MIT and the University of Illinois Open
// Source Licenses. See LICENSE.TXT for details.
//
//===----------------------------------------------------------------------===//

#ifndef EMPLACEABLE_H
#define EMPLACEABLE_H

#ifndef _LIBCPP_HAS_NO_RVALUE_REFERENCES

class Emplaceable
{
#if !defined(__clang__)
// GCC 4.8 when compile ccontainers/unord/unord.map/unorder.map.modifiers/emplace_hint.pass.cpp, etc,
// complains about the following being private
public:
    Emplaceable(const Emplaceable&) {}
#else
    Emplaceable(const Emplaceable&);
#endif
    Emplaceable& operator=(const Emplaceable&);

    int int_;
    double double_;
public:
    Emplaceable() : int_(0), double_(0) {}
    Emplaceable(int i, double d) : int_(i), double_(d) {}
    Emplaceable(Emplaceable&& x)
        : int_(x.int_), double_(x.double_)
            {x.int_ = 0; x.double_ = 0;}
    Emplaceable& operator=(Emplaceable&& x)
        {int_ = x.int_; x.int_ = 0;
         double_ = x.double_; x.double_ = 0;
         return *this;}

    bool operator==(const Emplaceable& x) const
        {return int_ == x.int_ && double_ == x.double_;}
    bool operator<(const Emplaceable& x) const
        {return int_ < x.int_ || (int_ == x.int_ && double_ < x.double_);}

    int get() const {return int_;}
};

namespace std {

template <>
struct hash<Emplaceable>
    : public std::unary_function<Emplaceable, std::size_t>
{
    std::size_t operator()(const Emplaceable& x) const {return x.get();}
};

}

#endif  // _LIBCPP_HAS_NO_RVALUE_REFERENCES

#endif  // EMPLACEABLE_H
