extern int foo(int);
extern int bar(int);

int main(void) {
  return foo(bar(10));
}
