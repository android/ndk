namespace cpp {

extern int foo2(int);

int foo3(int x) {
  return foo2(x) + 3;
}

}  // namespace cpp