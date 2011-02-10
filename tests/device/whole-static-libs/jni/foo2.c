/* This function is not called from main() but should still be included
 * in the final executable due to our use of LOCAL_WHOLE_STATIC_LIBRARIES.
 * main() will dlopen() itself to look for the function symbol.
 */
int foo2(int x)
{
    return x*2;
}
