#include <stdio.h>

// PR c++/15097
// { dg-do run }

typedef __SIZE_TYPE__ size_t;

extern "C" void * malloc (size_t);
extern "C" void free (void *);
extern "C" void abort(void);

void *saved;

void * operator new (size_t size)
{
  void *p = malloc (size);
  saved = p;
  return p;
}

void operator delete (void *p)
{
// Note that since STL is built w/o -Bsymbolic a lots of other code in STL
// may use the customized new/delete in this file.  We only save one copy of
// poitner in variable "saved", but because this testcase new and immediately
// delete in single thread, this single-copy serves well, provided that we only
// check saved==p once and set saved to NULL to prevent further comparison of
// unrelated delete from within STL
  if (saved) {
     if (p == saved) {
        saved = NULL;
     } else {
        abort ();
     }
  }

  free (p);
}

struct B1
{
    virtual ~B1 () throw() {}
    B1 (){}
    int x;
};
struct B2
{
    virtual ~B2 () throw() {}
    B2 (){}
    int x;
};
struct D : B1, B2
{
    D (){}
    ~D () throw() {}
    int y;
};
void f1 (D*);
void f2 (B2*);
void f3 (B1*);
int main (void)
{
    f1 (::new D);
    f2 (::new D);     
    f3 (::new D);
}
void f1 ( D* p) { ::delete p; }
void f2 (B2* p) { ::delete p; }  
void f3 (B1* p) { ::delete p; }
