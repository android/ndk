namespace cpp {

extern int foo3(int);

int foo4(int x) {
  return foo3(x) + 4;
}

}  // namespace cpp
