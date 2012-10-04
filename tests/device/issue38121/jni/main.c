#include <pthread.h>

static int testval = 4;
void dodgy_function(pthread_key_t* key)
{
    pthread_setspecific(*key, &testval);
}

int main(void)
{
    pthread_key_t mykey;
    pthread_key_create(&mykey, NULL);
    dodgy_function(&mykey);
    return 0;
}
