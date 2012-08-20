namespace cpp {
extern int foo6(int);
}

int x;

int main(int argc, char** argv) {
  x = cpp::foo6(10);
  return 0;
}
