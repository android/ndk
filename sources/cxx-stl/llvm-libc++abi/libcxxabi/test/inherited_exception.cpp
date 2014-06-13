//===--------------------- inherited_exception.cpp ------------------------===//
//
//                     The LLVM Compiler Infrastructure
//
// This file is dual licensed under the MIT and the University of Illinois Open
// Source Licenses. See LICENSE.TXT for details.
//
//===----------------------------------------------------------------------===//

#include <assert.h>

struct Base {
  int b1;
};

struct Base2 {
  int b2;
};

struct Child : public Base, public Base2 {
  int c;
};

void f1() {
  Child child;
  child.b1 = 10;
  child.b2 = 11;
  child.c = 12;
  throw child;
}

void f2() {
  Child child;
  child.b1 = 10;
  child.b2 = 11;
  child.c = 12;
  throw static_cast<Base2&>(child);
}

void f3() {
  Child* child  = new Child;
  child->b1 = 10;
  child->b2 = 11;
  child->c = 12;
  throw static_cast<Base2*>(child);
}

int main()
{
    try
    {
        f1();
        assert(false);
    }
    catch (const Child& c)
    {
        assert(true);
    }
    catch (const Base& b)
    {
        assert(false);
    }
    catch (...)
    {
        assert(false);
    }

    try
    {
        f1();
        assert(false);
    }
    catch (const Base& c)
    {
        assert(true);
    }
    catch (const Child& b)
    {
        assert(false);
    }
    catch (...)
    {
        assert(false);
    }

    try
    {
        f1();
        assert(false);
    }
    catch (const Base2& c)
    {
        assert(true);
    }
    catch (const Child& b)
    {
        assert(false);
    }
    catch (...)
    {
        assert(false);
    }

    try
    {
        f2();
        assert(false);
    }
    catch (const Child& c)
    {
        assert(false);
    }
    catch (const Base& b)
    {
        assert(false);
    }
    catch (const Base2& b)
    {
        assert(true);
    }
    catch (...)
    {
        assert(false);
    }

    try
    {
        f3();
        assert(false);
    }
    catch (const Base* c)
    {
        assert(false);
    }
    catch (const Child* b)
    {
        assert(false);
    }
    catch (const Base2* c)
    {
        assert(true);
    }
    catch (...)
    {
        assert(false);
    }
}
