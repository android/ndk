/* For details, see README */

extern "C" {

void __attribute__((constructor))
my_constructor(void)
{
    /* nothing */
}

void __attribute__((destructor))
my_destructor(void)
{
    /* nothing */
}

}

class Foo {
public:
    Foo() : mValue(1) {}
    ~Foo() { mValue = 0; }
private:
    int mValue;
};

static Foo foo;
