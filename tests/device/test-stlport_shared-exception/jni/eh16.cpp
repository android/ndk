// { dg-do run { xfail sparc64-*-elf arm-*-pe } }
// { dg-options "-fexceptions" }

int err = 1;

struct A {
  ~A() {
    --err;
  }
};

struct B {
  A a;
  B() {
    throw 1;
  }
};

int main() {
  try {
    B b;
  } catch (...) {
    return err;
  }
  return 1;
}
