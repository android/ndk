extern int bar(void);

int main(void) {
  return (bar() == 42) ? 0 : 1;
}
