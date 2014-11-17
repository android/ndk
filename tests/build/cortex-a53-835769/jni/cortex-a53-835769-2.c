int
foo (int a, int b, int c, int d, int e)
{
int result;
__asm__ __volatile ("// %0, %1"
: "=r" (result)
: "r" (b)
: /* No clobbers */
);
return c * d + e;
}

int main()
{
   
}
