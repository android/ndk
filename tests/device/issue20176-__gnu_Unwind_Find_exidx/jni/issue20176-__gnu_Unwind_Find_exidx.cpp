#include <new>
#include <exception>
#include <cstdio>

/* Stubbed out in libdl and defined in the dynamic linker.
 * Same semantics as __gnu_Unwind_Find_exidx().
 */
typedef long unsigned int *_Unwind_Ptr;
extern "C" _Unwind_Ptr dl_unwind_find_exidx(_Unwind_Ptr pc, int *pcount);
extern "C" _Unwind_Ptr __gnu_Unwind_Find_exidx(_Unwind_Ptr pc, int *pcount)
{
    _Unwind_Ptr ret_pc;
    printf("%p -> ", pc);
    ret_pc = dl_unwind_find_exidx(pc, pcount);
    printf("%p %d\n", ret_pc, *pcount);
    return ret_pc;
}
static void* g_func_ptr;

static void foo(void)
{
    try
    {
        ::printf("Hello ");
        throw std::exception();
    } catch (const std::exception &e)
    {
        ::printf(" World!\n");
    }
}

int main(int argc, char** argv)
{
    int count;
    g_func_ptr = (void*)__gnu_Unwind_Find_exidx;

    __gnu_Unwind_Find_exidx((_Unwind_Ptr)main, &count); // This one succeed

    // This one crash on Android <= 2.1.
    // The lcoal __gnu_Unwind_Find_exidx() isn't even called.
    // Need to recompile Android 2.1 from source and debug platform from there
    foo();
    return 0;
}
