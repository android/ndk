namespace cpp {

extern int foo4(int);

int foo5(int x) {
  return foo4(x) + 5;
}

}