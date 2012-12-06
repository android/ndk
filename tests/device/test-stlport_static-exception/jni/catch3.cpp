// { dg-do run  }
// { dg-options "-w" }
// Copyright (C) 1999, 2000 Free Software Foundation, Inc.
// Contributed by Nathan Sidwell 29 Aug 1999 <nathan@acm.org>

// We cannot catch an ambiguous base class. Check with a non-virtual public
// DAG.
// -- public, << private, == virtual

// D--B--A
// +--C--A

#include <cstdio>

struct A { int m; };
struct B : A { int m; };
struct C : A { int m; };
struct D : B, C { int m; };

void fna(A *obj) { throw obj; }
void fnb(B *obj) { throw obj; }
void fnc(C *obj) { throw obj; }
void fnd(D *obj) { throw obj; }

extern "C" void abort();

void check(D *d)
{
  //fprintf(stderr, "D: %p\n", d);
  //fprintf(stderr, "B: %p\n", (B*)d);
  //fprintf(stderr, "A: %p\n", (A*)(B*)d);
  //fprintf(stderr, "C: %p\n", (C*)d);
  //fprintf(stderr, "A: %p\n", (A*)(C*)d);
  int caught;
  
  // try with whole object
  caught = 0;
  try { fnd(d); }
  catch(A *p) { abort(); } // A is ambiguous
  catch(D *p) { caught = 1; if (p != d) abort();}
  catch(...) { abort(); }
  if (!caught) abort();

  caught = 0;
  try { fnd(d); }
  catch(A *p) { abort(); } // A is ambiguous
  catch(B *p) { caught = 1; if (p != d) abort();}
  catch(...) { abort(); }
  if (!caught) abort();

  caught = 0;
  try { fnd(d); }
  catch(A *p) { abort(); } // A is ambiguous
  catch(C *p) { caught = 1; if (p != d) abort();}
  catch(...) { abort(); }
  if (!caught) abort();

  // try with an A object
  caught = 0;
  try { fna((B *)d); }
  catch(B *p) { abort(); } // throw type is static type
  catch(A *p) { caught = 1; if (p != (B *)d) abort();}
  catch(...) { abort(); }
  if (!caught) abort();
  
  caught = 0;
  try { fna((C *)d); }
  catch(C *p) { abort(); } // throw type is static type
  catch(A *p) { caught = 1; if (p != (C *)d) abort();}
  catch(...) { abort(); }
  if (!caught) abort();
  
  // try with B object
  caught = 0;
  try { fnb((B *)d); }
  catch(A *p) { caught = 1; if (p != (B *)d) abort();}
  catch(...) { abort(); }
  if (!caught) abort();
  
  caught = 0;
  try { fnb((B *)d); }
  catch(B *p) { caught = 1; if (p != (B *)d) abort();}
  catch(...) { abort(); }
  if (!caught) abort();
  
  caught = 0;
  try { fnb((B *)d); }
  catch(C *p) { abort(); }
  catch(D *p) { abort(); }
  catch(...) { caught =1; }
  if (!caught) abort();
  
  // try with C object
  caught = 0;
  try { fnc((C *)d); }
  catch(A *p) { caught = 1; if (p != (C *)d) abort();}
  catch(...) { abort(); }
  if (!caught) abort();
  
  caught = 0;
  try { fnc((C *)d); }
  catch(C *p) { caught = 1; if (p != d) abort();}
  catch(...) { abort(); }
  if (!caught) abort();
  
  caught = 0;
  try { fnc((C *)d); }
  catch(B *p) { abort(); }
  catch(D *p) { abort(); }
  catch(...) { caught =1; }
  if (!caught) abort();
  
  return;
}

int main ()
{
  D d;
  check (&d); // try with an object
  check ((D *)0); // try with no object
  
  return 0;
}
