#include <cstdio>

class Foo
{
public:
  virtual ~Foo() { }
  virtual void print()
  {
    std::printf("in Foo!\n");
  }
};

class Bar: public Foo
{
  public:
  void print()
  {
    std::printf("in Bar!\n");
  }
};

int main()
{
    Foo* foo = new Bar();
    Bar* bar;

    bar = dynamic_cast<Bar*>(foo);
    if (bar != NULL) {
        printf("OK: 'foo' is pointing to a Bar class instance.\n");
    } else {
        fprintf(stderr, "KO: Could not dynamically cast 'foo' to a 'Bar*'\n");
        return 1;
    }

    delete foo;
    return 0;
}
