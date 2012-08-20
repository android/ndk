namespace cpp {

extern int foo5(int);

int foo6(int x) {
  return foo5(x) + 6;
}

}  // namespace cpp