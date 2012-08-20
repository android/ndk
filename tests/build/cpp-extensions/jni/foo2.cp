namespace cpp {

extern int foo1(int);

int foo2(int x) {
  return foo1(x) + 2;
}

}  // namespace cpp
