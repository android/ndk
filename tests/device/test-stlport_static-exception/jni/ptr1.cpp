// { dg-do run  }
// Bug: catching pointers by reference doesn't work right.

extern "C" int printf (const char *, ...);

struct E {
  int x;
  E(int i) { x = i; }
};

int main()
{
  try {
    E *p = new E(5);
    throw p;
  }

  catch (E *&e) {
    printf ("address of e is 0x%lx\n", (__SIZE_TYPE__)e);
    printf ("(__SIZE_TYPE__)e: %d ( != 5 ?)\n", (__SIZE_TYPE__)e);
    printf ("            e->x: %d ( == 5 ?)\n", e->x);
    return !((__SIZE_TYPE__)e != 5 && e->x == 5);
  }
  return 2;
}
